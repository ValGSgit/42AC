# 05 — Bonus: ft_atoi_base and the list functions

Reminder: the bonus is only graded if the mandatory part is **perfect**. Bonus code goes in `*_bonus.s` files, built by the `bonus` Makefile rule.

`ft_simd_memchr` gets its own doc: [06_SIMD_MEMCHR.md](06_SIMD_MEMCHR.md).

## The list structure in memory

```c
typedef struct s_list
{
    void          *data;   // offset 0, 8 bytes
    struct s_list *next;   // offset 8, 8 bytes
} t_list;                  // sizeof = 16
```

So with a node pointer in `rdi`: `[rdi]` is `data`, `[rdi + 8]` is `next`. With a `t_list **begin_list` in `rdi`: `[rdi]` is the head pointer.

Tip: give the offsets names so your code reads like C:

```nasm
%define DATA 0
%define NEXT 8
    mov     rdi, [rdi + NEXT]     ; node = node->next
```

(Or use NASM's `struc`/`endstruc`, see the NASM manual, section "STRUC and ENDSTRUC".)

## Calling through a function pointer

`ft_list_sort` and `ft_list_remove_if` receive `cmp` and `free_fct` as arguments. A function pointer is just an address in a register. To call it: `call r12` (if `r12` holds it). This is an **indirect call**: no `wrt ..plt`, no `extern`, the address is already known at runtime.

Everything from [doc 2](02_CALLING_CONVENTION.md) still applies, and here it matters a lot because you call *unknown* code inside a loop:
- Put `cmp`, `free_fct`, `data_ref`, `begin_list` and your loop variables in **callee-saved** registers. The callee is allowed to destroy every scratch register, and `ft_strcmp`'s replacement in the evaluator's test might do exactly that.
- Count your pushes for alignment.
- `cmp` returns an `int`: test `eax`, not `rax` (the upper half is garbage), and use **signed** jumps (`jg`, `jle`...).

---

## ft_list_size

```c
unsigned int ft_list_size(t_list *begin_list);
```

**Plan:** counter = 0; while node != NULL: counter++, node = node->next.

**Traps:** an empty list (`NULL`) returns 0. Return type is `unsigned int`, so the result goes in `eax`.

**Tests:** `NULL`, one node, many nodes.

---

## ft_list_push_front

```c
void ft_list_push_front(t_list **begin_list, void *data);
```

**Plan:**
1. Keep `begin_list` and `data` in callee-saved registers (you're about to call `malloc`).
2. `malloc(16)`.
3. If `NULL`, return (nothing else you can do; the subject doesn't say, so not crashing is enough).
4. `new->data = data; new->next = *begin_list; *begin_list = new;`

**Traps:**
- `begin_list == NULL` would crash on step 4. Decide whether to guard it (cheap: return early) and be ready to justify.
- Two saved registers = two pushes = misaligned stack. Fix it.
- `call malloc wrt ..plt`.

**Tests:** push onto an empty list (`t_list *head = NULL; ft_list_push_front(&head, "a");`), push several, check order (last pushed is first) and that `head` changed. Free everything, valgrind clean.

---

## ft_list_sort

```c
void ft_list_sort(t_list **begin_list, int (*cmp)());
```

"Ascending" means: after sorting, for every node, `cmp(node->data, node->next->data) <= 0`.

**Simplest correct approach: bubble sort that swaps `data`, not nodes.**

```
if begin_list == NULL or *begin_list == NULL: return
repeat:
    swapped = false
    node = *begin_list
    while node->next != NULL:
        if cmp(node->data, node->next->data) > 0:
            swap node->data and node->next->data
            swapped = true
        node = node->next
until not swapped
```

Swapping the `data` pointers avoids relinking nodes entirely, so the head pointer never changes and there are no pointer bugs. It's O(n²), which is fine here. If you want to impress: merge sort on linked lists is O(n log n) and doesn't need random access, but it's much harder to write in asm. Your choice, defend it.

**Registers you need to survive the `cmp` call:** `begin_list` (to restart), `cmp`, `node`, `swapped`. That's four callee-saved registers. Four pushes = even = misaligned. Plan for it.

**Traps:**
- `cmp` result: `test eax, eax` + `jg`/`jle`, not `rax`, not unsigned jumps.
- After the call, reload `node->next` from memory. Don't trust a scratch register you loaded before the call.

**Tests:** empty list, one element, already sorted, reverse sorted, duplicates, with `ft_strcmp` (or `strcmp`) as `cmp`. Also with a custom `cmp` on integers stored in `data`, e.g. `(int)(intptr_t)a - (int)(intptr_t)b`, to check you're not secretly assuming strings.

---

## ft_list_remove_if

```c
void ft_list_remove_if(t_list **begin_list, void *data_ref,
                       int (*cmp)(), void (*free_fct)(void *));
```

Removes every node where `cmp(node->data, data_ref) == 0`: free the data with `free_fct`, then the node with `free`.

**The hard part is removing the head.** Removing the first node means changing `*begin_list`; removing any other node means changing the previous node's `next`. Two cases, two code paths, double the bugs.

**The trick that makes it one case: walk a pointer to the link, not a pointer to the node.**

```
link = begin_list                  // t_list **: points to "the pointer that points to the current node"
while *link != NULL:
    node = *link
    if cmp(node->data, data_ref) == 0:
        *link = node->next         // unhook: works for the head AND for the middle
        free_fct(node->data)
        free(node)
        // don't advance: *link is now the next node, check it too
    else:
        link = &node->next         // = node + 8
```

`&node->next` is just `node + 8` because `next` is at offset 8: one `lea`. At the start `link` points into the caller's variable (`&head`), afterwards it points into a node's `next` field. Both are "a place holding a `t_list *`", so the same code handles both. (Linus Torvalds calls this "good taste" in a well-known TED interview; search "Linus good taste linked list".)

**Registers to keep across calls:** `link`, `data_ref`, `cmp`, `free_fct`, and `node` (needed after `free_fct` to call `free(node)`). Five callee-saved registers: count the pushes and check whether you need extra alignment.

**Traps:**
- Read `node->next` **before** `free(node)`. Reading freed memory "works" until it doesn't; valgrind will catch it.
- `call free wrt ..plt` (libc), but `call r15` (or whichever register holds `free_fct`) for the user's function.
- Consecutive matching nodes, all nodes matching (the list must end up `NULL`), none matching.
- `begin_list == NULL`: guard or not, decide and justify.

**Tests:** list of `strdup`'d strings, `cmp = strcmp`, `free_fct = free`; remove the head, the tail, a middle node, two in a row, all of them, none of them. Valgrind: no leaks, no invalid reads.

---

## ft_atoi_base

```c
int ft_atoi_base(char *str, char *base);
```

### Step 1: validate the base

Return 0 if:
- `base` has fewer than 2 characters,
- a character appears twice,
- it contains `+`, `-`, or whitespace (`' '`, `'\t'`, `'\n'`, `'\v'`, `'\f'`, `'\r'`, i.e. 32 and 9–13).

Duplicates: the simple way is a nested loop (for each char, compare with every later char), O(len²), totally fine for bases of at most 256 characters.

**A neater way (not obvious):** build a 256-byte lookup table on the stack (`sub rsp, 256`, keep alignment in mind), filled with -1 (`rep stosb` with `al = 0xFF`). For each base char `c` at index `i`: if `table[c] != -1` it's a duplicate, else `table[c] = i`. Then in step 3, converting a character is one lookup: `table[c]` is its digit value, or -1 if it's not in the base. Remember to give the 256 bytes back (`add rsp, 256`) on every return path.

### Step 2: whitespace and signs

"Behave exactly like `ft_atoi`" is ambiguous at 42, and you should pick one and say which:
- **libc `atoi` behavior:** skip whitespace, then *one* optional `+` or `-`.
- **Piscine `ft_atoi` behavior** (C 04): skip whitespace, then *any number* of `+`/`-`; the result is negative if there's an odd number of `-`. This is the usual expectation for `ft_atoi_base` because it comes from the same Piscine exercise.

### Step 3: digits

```
result = 0
while str[i] is in base:
    result = result * base_len + value_of(str[i])
    i++
return sign * result
```

Stop at the first character not in the base. Use `imul` for the multiply. Overflow behavior is undefined for `atoi` too, so wrapping like C `int` arithmetic is acceptable; just don't crash.

**Traps:**
- Load characters with `movzx` (they're used as table indexes, and `char` may be negative).
- The return value is an `int`: `eax`.
- Many registers in flight (str, base, base_len, sign, result, index). Leaf function (no calls), so scratch registers are plenty if you plan which holds what. Write that mapping in a comment at the top of the file.

**Tests:** `("42", "0123456789")` → 42, `("ff", "0123456789abcdef")` → 255, `("  \t-101", "01")` → -5, `("--+7", "0123456789")` → 7 (Piscine style), `("12a3", "0123456789")` → 12, invalid bases `""`, `"0"`, `"00"`, `"01+"`, `"0 1"` → 0, `("", "01")` → 0. Custom bases like `"poneyvif"`.
