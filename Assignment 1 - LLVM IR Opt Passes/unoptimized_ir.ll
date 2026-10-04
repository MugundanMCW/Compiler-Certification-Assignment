; ============================================================
; LLVM IR test file for HelloWorldPass
;
; Pass:
;   - Constant Folding
;   - Strength Reduction
;   - Common Subexpression Elimination (CSE)
;   - Dead Code Elimination (DCE)
;
; Run:
;
;   opt.exe -passes="helloworld" test.ll -S -o optimized.ll
;
; ============================================================


; ============================================================
; 1. CONSTANT FOLDING
;
; Expected:
;
;   10 + 20 -> 30
;   30 * 4  -> 120
;
; ============================================================

define i32 @test_constant_folding() {
entry:
    %add = add i32 10, 20
    %mul = mul i32 %add, 4
    ret i32 %mul
}


; ============================================================
; 2. STRENGTH REDUCTION - MULTIPLICATION BY 1
;
; Expected:
;
;   x * 1 -> x
;
; ============================================================

define i32 @test_mul_one(i32 %x) {
entry:
    %result = mul i32 %x, 1
    ret i32 %result
}


; ============================================================
; 3. STRENGTH REDUCTION - MULTIPLICATION BY POWER OF 2
;
; Expected:
;
;   x * 8 -> x << 3
;
; ============================================================

define i32 @test_mul_power_of_two(i32 %x) {
entry:
    %result = mul i32 %x, 8
    ret i32 %result
}


; ============================================================
; 4. STRENGTH REDUCTION - ADDITION BY ZERO
;
; Expected:
;
;   x + 0 -> x
;
; ============================================================

define i32 @test_add_zero(i32 %x) {
entry:
    %result = add i32 %x, 0
    ret i32 %result
}


; ============================================================
; 5. STRENGTH REDUCTION - SUBTRACTION BY ZERO
;
; Expected:
;
;   x - 0 -> x
;
; ============================================================

define i32 @test_sub_zero(i32 %x) {
entry:
    %result = sub i32 %x, 0
    ret i32 %result
}


; ============================================================
; 6. STRENGTH REDUCTION - DIVISION BY 1
;
; Expected:
;
;   x / 1 -> x
;
; ============================================================

define i32 @test_div_one(i32 %x) {
entry:
    %result = sdiv i32 %x, 1
    ret i32 %result
}


; ============================================================
; 7. STRENGTH REDUCTION - DIVISION BY POWER OF 2
;
; Expected:
;
;   x / 8 -> x >> 3
;
; Note:
; The current pass uses AShr for signed division.
; ============================================================

define i32 @test_div_power_of_two(i32 %x) {
entry:
    %result = sdiv i32 %x, 8
    ret i32 %result
}


; ============================================================
; 8. CSE - COMMON SUBEXPRESSION ELIMINATION
;
; The same expression is calculated twice:
;
;   x + y
;   x + y
;
; Expected:
;
;   The second addition is replaced by the first result.
;
; ============================================================

define i32 @test_cse(i32 %x, i32 %y) {
entry:
    %a = add i32 %x, %y
    %b = add i32 %x, %y
    %result = add i32 %a, %b
    ret i32 %result
}


; ============================================================
; 9. CSE - MULTIPLICATION
;
; Same multiplication appears twice.
;
; Expected:
;
;   %b should reuse %a.
;
; ============================================================

define i32 @test_cse_mul(i32 %x, i32 %y) {
entry:
    %a = mul i32 %x, %y
    %b = mul i32 %x, %y
    %result = add i32 %a, %b
    ret i32 %result
}


; ============================================================
; 10. DEAD CODE ELIMINATION
;
; %unused is never used.
;
; Expected:
;
;   %unused is removed.
;
; ============================================================

define i32 @test_dce(i32 %x) {
entry:
    %unused = add i32 %x, 100
    %result = add i32 %x, 10
    ret i32 %result
}


; ============================================================
; 11. DEAD CODE + CONSTANT FOLDING
;
; Expected:
;
;   100 + 200 -> 300
;   But the result is unused.
;
; Therefore DCE should remove it.
;
; ============================================================

define i32 @test_constant_and_dce(i32 %x) {
entry:
    %unused = add i32 100, 200
    %result = add i32 %x, 10
    ret i32 %result
}


; ============================================================
; 12. COMBINED OPTIMIZATION TEST
;
; This exercises multiple transformations.
;
; Expected transformations:
;
;   %mul1 = x * 8
;          -> x << 3
;
;   %add1 = %mul1 + 0
;          -> %mul1
;
;   %mul2 = x * 8
;          -> x << 3
;
;   CSE may eliminate the duplicated expression depending
;   on the order in which the custom passes execute.
;
;   %unused = 100 + 200
;            -> 300
;            -> removed by DCE
;
; ============================================================

define i32 @test_combined(i32 %x) {
entry:
    %mul1 = mul i32 %x, 8
    %add1 = add i32 %mul1, 0

    %mul2 = mul i32 %x, 8

    %result = add i32 %add1, %mul2

    %unused = add i32 100, 200

    ret i32 %result
}


; ============================================================
; 13. MULTIPLE CONSTANT OPERATIONS
;
; Expected:
;
;   10 + 20 -> 30
;   30 * 4  -> 120
;   120 - 20 -> 100
;
; ============================================================

define i32 @test_multiple_constant_folding() {
entry:
    %a = add i32 10, 20
    %b = mul i32 %a, 4
    %c = sub i32 %b, 20
    ret i32 %c
}


; ============================================================
; 14. MULTIPLE STRENGTH REDUCTIONS
;
; Expected:
;
;   x * 4 -> x << 2
;   result + 0 -> result
;   result - 0 -> result
;
; ============================================================

define i32 @test_multiple_strength_reductions(i32 %x) {
entry:
    %a = mul i32 %x, 4
    %b = add i32 %a, 0
    %c = sub i32 %b, 0
    ret i32 %c
}


; ============================================================
; 15. CSE + STRENGTH REDUCTION
;
; Two identical multiplication expressions.
;
; Expected:
;
;   x * 16 -> x << 4
;
; CSE can eliminate the duplicate computation.
;
; ============================================================

define i32 @test_cse_and_strength_reduction(i32 %x) {
entry:
    %a = mul i32 %x, 16
    %b = mul i32 %x, 16
    %result = add i32 %a, %b
    ret i32 %result
}
