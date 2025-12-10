; ModuleID = 'vulpes_module'
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

declare i32 @printf(i8*, ...)
declare i32 @scanf(i8*, ...)
declare double @sqrt(double)
declare i64 @time(i8*)

@.str_int = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1
@.str_float = private unnamed_addr constant [4 x i8] c"%g\0A\00", align 1
@.str_string = private unnamed_addr constant [4 x i8] c"%s\0A\00", align 1
@.str_input_int = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str_input_float = private unnamed_addr constant [4 x i8] c"%lf\00", align 1
@rand_seed = global i32 1, align 4
@rand_seeded = global i1 false, align 1

@.str1 = private unnamed_addr constant [15 x i8] c"======%s=====\0A\00", align 1
@.str2 = private unnamed_addr constant [11 x i8] c"Arithmetic\00", align 1
@.str3 = private unnamed_addr constant [12 x i8] c"a + b = %d\0A\00", align 1
@.str4 = private unnamed_addr constant [12 x i8] c"a - b = %d\0A\00", align 1
@.str5 = private unnamed_addr constant [12 x i8] c"a * b = %d\0A\00", align 1
@.str6 = private unnamed_addr constant [12 x i8] c"a / b = %d\0A\00", align 1
@.str7 = private unnamed_addr constant [19 x i8] c"golden ratio = %g\0A\00", align 1
@.str8 = private unnamed_addr constant [12 x i8] c"Comparisons\00", align 1
@.str9 = private unnamed_addr constant [18 x i8] c"lhs == rhs -> %d\0A\00", align 1
@.str10 = private unnamed_addr constant [18 x i8] c"lhs != rhs -> %d\0A\00", align 1
@.str11 = private unnamed_addr constant [18 x i8] c"lhs < 5    -> %d\0A\00", align 1
@.str12 = private unnamed_addr constant [18 x i8] c"lhs <= 4   -> %d\0A\00", align 1
@.str13 = private unnamed_addr constant [18 x i8] c"lhs > 3    -> %d\0A\00", align 1
@.str14 = private unnamed_addr constant [18 x i8] c"lhs >= 5   -> %d\0A\00", align 1
@.str15 = private unnamed_addr constant [6 x i8] c"Loops\00", align 1
@.str16 = private unnamed_addr constant [15 x i8] c"sum 0..4 = %d\0A\00", align 1
@.str17 = private unnamed_addr constant [10 x i8] c"count %d\0A\00", align 1
@.str18 = private unnamed_addr constant [17 x i8] c"Namespaced Calls\00", align 1
@.str19 = private unnamed_addr constant [25 x i8] c"math.adder(%d, %d) = %d\0A\00", align 1
@.str20 = private unnamed_addr constant [24 x i8] c"math.scaler(2, 4) = %d\0A\00", align 1
@.str21 = private unnamed_addr constant [26 x i8] c"Running Vulpes test suite\00", align 1
define i32 @math_adder(i32 %a, i32 %b) {
entry:
  %t1 = alloca i32, align 4
  store i32 %a, i32* %t1, align 4
  %t2 = alloca i32, align 4
  store i32 %b, i32* %t2, align 4
  %t3 = load i32, i32* %t1, align 4
  %t4 = load i32, i32* %t2, align 4
  %t5 = add i32 %t3, %t4
  ret i32 %t5
}

define i32 @math_scaler(i32 %value, i32 %times) {
entry:
  %t6 = alloca i32, align 4
  store i32 %value, i32* %t6, align 4
  %t7 = alloca i32, align 4
  store i32 %times, i32* %t7, align 4
  %t8 = alloca i32, align 4
  store i32 0, i32* %t8, align 4
  %t9 = load i32, i32* %t7, align 4
  %t10 = alloca i32, align 4
  store i32 0, i32* %t10, align 4
  br label %for_cond_1
for_cond_1:
  %t11 = load i32, i32* %t10, align 4
  %t12 = icmp slt i32 %t11, %t9
  br i1 %t12, label %for_body_2, label %for_end_3
for_body_2:
  %t13 = load i32, i32* %t8, align 4
  %t14 = load i32, i32* %t6, align 4
  %t15 = add i32 %t13, %t14
  store i32 %t15, i32* %t8, align 4
  %t16 = add i32 %t11, 1
  store i32 %t16, i32* %t10, align 4
  br label %for_cond_1
for_end_3:
  %t17 = load i32, i32* %t8, align 4
  ret i32 %t17
}

define double @goldenRatio() {
entry:
  %t18 = sitofp i32 5 to double
  %t19 = call double @sqrt(double %t18)
  %t20 = sitofp i32 1 to double
  %t21 = fadd double %t20, %t19
  %t22 = sitofp i32 2 to double
  %t23 = fdiv double %t21, %t22
  ret double %t23
}

define void @coolPrint(i8* %s) {
entry:
  %t24 = alloca i8*, align 8
  store i8* %s, i8** %t24, align 8
  %t25 = load i8*, i8** %t24, align 8
  %t26 = getelementptr inbounds [15 x i8], [15 x i8]* @.str1, i32 0, i32 0
  %t27 = call i32 (i8*, ...) @printf(i8* %t26, i8* %t25)
  ret void
}

define void @testArithmetic() {
entry:
  %t28 = getelementptr inbounds [11 x i8], [11 x i8]* @.str2, i32 0, i32 0
  call void @coolPrint(i8* %t28)
  %t29 = alloca i32, align 4
  store i32 10, i32* %t29, align 4
  %t30 = alloca i32, align 4
  store i32 3, i32* %t30, align 4
  %t31 = load i32, i32* %t29, align 4
  %t32 = load i32, i32* %t30, align 4
  %t33 = add i32 %t31, %t32
  %t34 = getelementptr inbounds [12 x i8], [12 x i8]* @.str3, i32 0, i32 0
  %t35 = call i32 (i8*, ...) @printf(i8* %t34, i32 %t33)
  %t36 = load i32, i32* %t29, align 4
  %t37 = load i32, i32* %t30, align 4
  %t38 = sub i32 %t36, %t37
  %t39 = getelementptr inbounds [12 x i8], [12 x i8]* @.str4, i32 0, i32 0
  %t40 = call i32 (i8*, ...) @printf(i8* %t39, i32 %t38)
  %t41 = load i32, i32* %t29, align 4
  %t42 = load i32, i32* %t30, align 4
  %t43 = mul i32 %t41, %t42
  %t44 = getelementptr inbounds [12 x i8], [12 x i8]* @.str5, i32 0, i32 0
  %t45 = call i32 (i8*, ...) @printf(i8* %t44, i32 %t43)
  %t46 = load i32, i32* %t29, align 4
  %t47 = load i32, i32* %t30, align 4
  %t48 = sdiv i32 %t46, %t47
  %t49 = getelementptr inbounds [12 x i8], [12 x i8]* @.str6, i32 0, i32 0
  %t50 = call i32 (i8*, ...) @printf(i8* %t49, i32 %t48)
  %t51 = call double @goldenRatio()
  %t52 = alloca double, align 8
  store double %t51, double* %t52, align 8
  %t53 = load double, double* %t52, align 8
  %t54 = getelementptr inbounds [19 x i8], [19 x i8]* @.str7, i32 0, i32 0
  %t55 = call i32 (i8*, ...) @printf(i8* %t54, double %t53)
  ret void
}

define void @testComparisons() {
entry:
  %t56 = getelementptr inbounds [12 x i8], [12 x i8]* @.str8, i32 0, i32 0
  call void @coolPrint(i8* %t56)
  %t57 = alloca i32, align 4
  store i32 4, i32* %t57, align 4
  %t58 = alloca i32, align 4
  store i32 4, i32* %t58, align 4
  %t59 = load i32, i32* %t57, align 4
  %t60 = load i32, i32* %t58, align 4
  %t61 = icmp eq i32 %t59, %t60
  %t62 = getelementptr inbounds [18 x i8], [18 x i8]* @.str9, i32 0, i32 0
  %t63 = zext i1 %t61 to i32
  %t64 = call i32 (i8*, ...) @printf(i8* %t62, i32 %t63)
  %t65 = load i32, i32* %t57, align 4
  %t66 = load i32, i32* %t58, align 4
  %t67 = icmp ne i32 %t65, %t66
  %t68 = getelementptr inbounds [18 x i8], [18 x i8]* @.str10, i32 0, i32 0
  %t69 = zext i1 %t67 to i32
  %t70 = call i32 (i8*, ...) @printf(i8* %t68, i32 %t69)
  %t71 = load i32, i32* %t57, align 4
  %t72 = icmp slt i32 %t71, 5
  %t73 = getelementptr inbounds [18 x i8], [18 x i8]* @.str11, i32 0, i32 0
  %t74 = zext i1 %t72 to i32
  %t75 = call i32 (i8*, ...) @printf(i8* %t73, i32 %t74)
  %t76 = load i32, i32* %t57, align 4
  %t77 = icmp sle i32 %t76, 4
  %t78 = getelementptr inbounds [18 x i8], [18 x i8]* @.str12, i32 0, i32 0
  %t79 = zext i1 %t77 to i32
  %t80 = call i32 (i8*, ...) @printf(i8* %t78, i32 %t79)
  %t81 = load i32, i32* %t57, align 4
  %t82 = icmp sgt i32 %t81, 3
  %t83 = getelementptr inbounds [18 x i8], [18 x i8]* @.str13, i32 0, i32 0
  %t84 = zext i1 %t82 to i32
  %t85 = call i32 (i8*, ...) @printf(i8* %t83, i32 %t84)
  %t86 = load i32, i32* %t57, align 4
  %t87 = icmp sge i32 %t86, 5
  %t88 = getelementptr inbounds [18 x i8], [18 x i8]* @.str14, i32 0, i32 0
  %t89 = zext i1 %t87 to i32
  %t90 = call i32 (i8*, ...) @printf(i8* %t88, i32 %t89)
  ret void
}

define void @testLoops() {
entry:
  %t91 = getelementptr inbounds [6 x i8], [6 x i8]* @.str15, i32 0, i32 0
  call void @coolPrint(i8* %t91)
  %t92 = alloca i32, align 4
  store i32 0, i32* %t92, align 4
  %t93 = alloca i32, align 4
  store i32 0, i32* %t93, align 4
  br label %for_cond_4
for_cond_4:
  %t94 = load i32, i32* %t93, align 4
  %t95 = icmp slt i32 %t94, 5
  br i1 %t95, label %for_body_5, label %for_end_6
for_body_5:
  %t96 = load i32, i32* %t92, align 4
  %t97 = load i32, i32* %t93, align 4
  %t98 = add i32 %t96, %t97
  store i32 %t98, i32* %t92, align 4
  %t99 = add i32 %t94, 1
  store i32 %t99, i32* %t93, align 4
  br label %for_cond_4
for_end_6:
  %t100 = load i32, i32* %t92, align 4
  %t101 = getelementptr inbounds [15 x i8], [15 x i8]* @.str16, i32 0, i32 0
  %t102 = call i32 (i8*, ...) @printf(i8* %t101, i32 %t100)
  %t103 = alloca i32, align 4
  store i32 3, i32* %t103, align 4
  br label %while_cond_7
while_cond_7:
  %t104 = load i32, i32* %t103, align 4
  %t105 = icmp sgt i32 %t104, 0
  br i1 %t105, label %while_body_8, label %while_end_9
while_body_8:
  %t106 = load i32, i32* %t103, align 4
  %t107 = getelementptr inbounds [10 x i8], [10 x i8]* @.str17, i32 0, i32 0
  %t108 = call i32 (i8*, ...) @printf(i8* %t107, i32 %t106)
  %t109 = load i32, i32* %t103, align 4
  %t110 = sub i32 %t109, 1
  store i32 %t110, i32* %t103, align 4
  br label %while_cond_7
while_end_9:
  ret void
}

define void @testNamespacedCalls() {
entry:
  %t111 = getelementptr inbounds [17 x i8], [17 x i8]* @.str18, i32 0, i32 0
  call void @coolPrint(i8* %t111)
  %t112 = alloca i32, align 4
  store i32 7, i32* %t112, align 4
  %t113 = load i32, i32* %t112, align 4
  %t114 = call i32 @math_adder(i32 %t113, i32 5)
  %t115 = alloca i32, align 4
  store i32 %t114, i32* %t115, align 4
  %t116 = load i32, i32* %t112, align 4
  %t117 = load i32, i32* %t115, align 4
  %t118 = getelementptr inbounds [25 x i8], [25 x i8]* @.str19, i32 0, i32 0
  %t119 = call i32 (i8*, ...) @printf(i8* %t118, i32 %t116, i32 5, i32 %t117)
  %t120 = call i32 @math_scaler(i32 2, i32 4)
  %t121 = alloca i32, align 4
  store i32 %t120, i32* %t121, align 4
  %t122 = load i32, i32* %t121, align 4
  %t123 = getelementptr inbounds [24 x i8], [24 x i8]* @.str20, i32 0, i32 0
  %t124 = call i32 (i8*, ...) @printf(i8* %t123, i32 %t122)
  ret void
}

define void @main() {
entry:
  %t125 = getelementptr inbounds [26 x i8], [26 x i8]* @.str21, i32 0, i32 0
  call void @coolPrint(i8* %t125)
  call void @testArithmetic()
  call void @testComparisons()
  call void @testLoops()
  call void @testNamespacedCalls()
  ret void
}

