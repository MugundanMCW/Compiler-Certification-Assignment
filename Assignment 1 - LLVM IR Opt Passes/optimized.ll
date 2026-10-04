; ModuleID = 'test.ll'
source_filename = "test.ll"

define i32 @test_constant_folding() {
entry:
  ret i32 120
}

define i32 @test_mul_one(i32 %x) {
entry:
  ret i32 %x
}

define i32 @test_mul_power_of_two(i32 %x) {
entry:
  %0 = shl i32 %x, 3
  ret i32 %0
}

define i32 @test_add_zero(i32 %x) {
entry:
  ret i32 %x
}

define i32 @test_sub_zero(i32 %x) {
entry:
  ret i32 %x
}

define i32 @test_div_one(i32 %x) {
entry:
  ret i32 %x
}

define i32 @test_div_power_of_two(i32 %x) {
entry:
  %0 = ashr i32 %x, 3
  ret i32 %0
}

define i32 @test_cse(i32 %x, i32 %y) {
entry:
  %a = add i32 %x, %y
  %result = add i32 %a, %a
  ret i32 %result
}

define i32 @test_cse_mul(i32 %x, i32 %y) {
entry:
  %a = mul i32 %x, %y
  %result = add i32 %a, %a
  ret i32 %result
}

define i32 @test_dce(i32 %x) {
entry:
  %result = add i32 %x, 10
  ret i32 %result
}

define i32 @test_constant_and_dce(i32 %x) {
entry:
  %result = add i32 %x, 10
  ret i32 %result
}

define i32 @test_combined(i32 %x) {
entry:
  %0 = shl i32 %x, 3
  %result = add i32 %0, %0
  ret i32 %result
}

define i32 @test_multiple_constant_folding() {
entry:
  ret i32 100
}

define i32 @test_multiple_strength_reductions(i32 %x) {
entry:
  %0 = shl i32 %x, 2
  ret i32 %0
}

define i32 @test_cse_and_strength_reduction(i32 %x) {
entry:
  %0 = shl i32 %x, 4
  %result = add i32 %0, %0
  ret i32 %result
}
