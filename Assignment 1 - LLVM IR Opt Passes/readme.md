# Custom LLVM Optimization Pass

This project contains a custom LLVM function pass that performs a few basic optimization techniques on LLVM IR.

The pass is implemented using LLVM's New Pass Manager.

## Build Instructions
clone the llvm source code: git clone --depth=1 https://github.com/llvm/llvm-project.git
Replace the Helloworld.cpp and Helloworld.h in the utils/transform folder
Build the project: cmake -S llvm -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_TARGETS_TO_BUILD="X86"
Build the opt: ninja -C build opt


## Optimizations

The pass currently performs three main optimizations:

### 1. Strength Reduction and Constant Folding

The pass simplifies arithmetic operations where possible.

#### Constant Folding

When both operands are constants, the operation is evaluated at compile time.

Examples:

```text
5 * 10  -> 50
5 + 10  -> 15
20 - 5  -> 15
20 / 5  -> 4
```

The pass handles constant folding for:

- Multiplication
- Addition
- Subtraction
- Division

#### Strength Reduction

Some arithmetic operations are replaced with simpler operations.

Examples:

```text
x * 1  -> x
1 * x  -> x
x + 0  -> x
0 + x  -> x
x - 0  -> x
x / 1  -> x
```

Multiplication by a power of two is converted into a left shift:

```text
x * 2  -> x << 1
x * 4  -> x << 2
x * 8  -> x << 3
```

Division by a positive power of two is converted into a right shift:

```text
x / 2  -> x >> 1
x / 4  -> x >> 2
x / 8  -> x >> 3
```

---

### 2. Common Subexpression Elimination

The pass looks for instructions that compute the same expression more than once.

For example:

```llvm
%1 = add i32 %a, %b
%2 = add i32 %a, %b
```

The second computation can be replaced with the result of the first:

```llvm
%1 = add i32 %a, %b
```

The CSE implementation handles both:

- Local common subexpressions within a basic block
- Common subexpressions across basic blocks using the Dominator Tree

The pass uses LLVM's `DominatorTreeAnalysis` to check whether the previous instruction is valid for the uses of the duplicate instruction.

---

### 3. Dead Code Elimination

The DCE part removes instructions whose results are no longer used and which do not have side effects.

For example:

```llvm
%1 = add i32 %a, %b
ret i32 %a
```

If `%1` is never used, it can be removed.

Instructions with side effects are preserved.

The pass also keeps:

- Instructions with users
- Instructions that may have side effects
- Terminator instructions
- Exception handling landing pads

---

## Pass Order

The optimizations are run in the following order:

```text
Strength Reduction + Constant Folding
                |
                v
               CSE
                |
                v
               DCE
                |
                v
          Additional DCE
```

The additional DCE pass is useful because the earlier optimizations can make some instructions dead.

For example, CSE may replace the uses of an instruction, making the original instruction removable.

---

## Example

### Input

```llvm
define i32 @example(i32 %a, i32 %b) {
entry:
    %x = mul i32 %a, 4
    %y = mul i32 %a, 4
    %z = add i32 %y, 0
    ret i32 %z
}
```

The pass can apply:

```text
a * 4 -> a << 2
```

CSE can remove the duplicate multiplication, and:

```text
x + 0 -> x
```

can remove the unnecessary addition.

The resulting IR is simplified to something similar to:

```llvm
define i32 @example(i32 %a, i32 %b) {
entry:
    %x = shl i32 %a, 2
    ret i32 %x
}
```

---

## Debug Output

The pass prints information about the transformations it performs.

### Dead Code Elimination

```text
[DCE] Removing: ...
```

### Local CSE

```text
[CSE] Local common subexpression: ...
```

### Global CSE

```text
[CSE] Global common subexpression: ...
```

### Constant Folding

```text
[CF] 5 * 10 -> 50
```

### Strength Reduction

```text
[SR] 4 * x -> x << 2
```

The pass also prints when the optimization pipeline starts and finishes for a function.

---

## Implementation

The main pass is:

```cpp
HelloWorldPass
```

The implementation contains three helper functions:

```cpp
runStrengthReductionAndConstantFolding()
runCSE()
runDCE()
```

These helper functions are kept inside `HelloWorld.cpp`, while `HelloWorld.h` only contains the `HelloWorldPass` declaration.

The pass uses LLVM's New Pass Manager interface:

```cpp
PreservedAnalyses HelloWorldPass::run(
    Function &F,
    FunctionAnalysisManager &AM);
```

---

## Files

```text
HelloWorld.h
HelloWorld.cpp
README.md
```

`HelloWorld.h` contains the pass declaration.

`HelloWorld.cpp` contains the implementation of the optimization passes.

`README.md` describes the optimizations and their behavior.

---

