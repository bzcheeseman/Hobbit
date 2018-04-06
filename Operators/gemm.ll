; ModuleID = '/Users/Aman/code/Hobbit/Operators/gemm.c'
source_filename = "/Users/Aman/code/Hobbit/Operators/gemm.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.12.0"

; Function Attrs: noinline nounwind ssp uwtable
define void @sgemm(float**, float**, float**, i64, i64, i64, i1 zeroext, i1 zeroext) #0 {
  %9 = alloca float**, align 8
  %10 = alloca float**, align 8
  %11 = alloca float**, align 8
  %12 = alloca i64, align 8
  %13 = alloca i64, align 8
  %14 = alloca i64, align 8
  %15 = alloca i8, align 1
  %16 = alloca i8, align 1
  %17 = alloca i64, align 8
  %18 = alloca i64, align 8
  %19 = alloca i64, align 8
  %20 = alloca float, align 4
  %21 = alloca float, align 4
  store float** %0, float*** %9, align 8
  store float** %1, float*** %10, align 8
  store float** %2, float*** %11, align 8
  store i64 %3, i64* %12, align 8
  store i64 %4, i64* %13, align 8
  store i64 %5, i64* %14, align 8
  %22 = zext i1 %6 to i8
  store i8 %22, i8* %15, align 1
  %23 = zext i1 %7 to i8
  store i8 %23, i8* %16, align 1
  store i64 0, i64* %17, align 8
  br label %24

; <label>:24:                                     ; preds = %98, %8
  %25 = load i64, i64* %17, align 8
  %26 = load i64, i64* %14, align 8
  %27 = icmp ult i64 %25, %26
  br i1 %27, label %28, label %101

; <label>:28:                                     ; preds = %24
  store i64 0, i64* %18, align 8
  br label %29

; <label>:29:                                     ; preds = %94, %28
  %30 = load i64, i64* %18, align 8
  %31 = load i64, i64* %12, align 8
  %32 = icmp ult i64 %30, %31
  br i1 %32, label %33, label %97

; <label>:33:                                     ; preds = %29
  store i64 0, i64* %19, align 8
  br label %34

; <label>:34:                                     ; preds = %90, %33
  %35 = load i64, i64* %19, align 8
  %36 = load i64, i64* %13, align 8
  %37 = icmp ult i64 %35, %36
  br i1 %37, label %38, label %93

; <label>:38:                                     ; preds = %34
  %39 = load i8, i8* %15, align 1
  %40 = trunc i8 %39 to i1
  br i1 %40, label %41, label %49

; <label>:41:                                     ; preds = %38
  %42 = load float**, float*** %10, align 8
  %43 = load i64, i64* %18, align 8
  %44 = getelementptr inbounds float*, float** %42, i64 %43
  %45 = load float*, float** %44, align 8
  %46 = load i64, i64* %17, align 8
  %47 = getelementptr inbounds float, float* %45, i64 %46
  %48 = load float, float* %47, align 4
  br label %57

; <label>:49:                                     ; preds = %38
  %50 = load float**, float*** %10, align 8
  %51 = load i64, i64* %17, align 8
  %52 = getelementptr inbounds float*, float** %50, i64 %51
  %53 = load float*, float** %52, align 8
  %54 = load i64, i64* %18, align 8
  %55 = getelementptr inbounds float, float* %53, i64 %54
  %56 = load float, float* %55, align 4
  br label %57

; <label>:57:                                     ; preds = %49, %41
  %58 = phi float [ %48, %41 ], [ %56, %49 ]
  store float %58, float* %20, align 4
  %59 = load i8, i8* %16, align 1
  %60 = trunc i8 %59 to i1
  br i1 %60, label %61, label %69

; <label>:61:                                     ; preds = %57
  %62 = load float**, float*** %11, align 8
  %63 = load i64, i64* %17, align 8
  %64 = getelementptr inbounds float*, float** %62, i64 %63
  %65 = load float*, float** %64, align 8
  %66 = load i64, i64* %19, align 8
  %67 = getelementptr inbounds float, float* %65, i64 %66
  %68 = load float, float* %67, align 4
  br label %77

; <label>:69:                                     ; preds = %57
  %70 = load float**, float*** %11, align 8
  %71 = load i64, i64* %19, align 8
  %72 = getelementptr inbounds float*, float** %70, i64 %71
  %73 = load float*, float** %72, align 8
  %74 = load i64, i64* %17, align 8
  %75 = getelementptr inbounds float, float* %73, i64 %74
  %76 = load float, float* %75, align 4
  br label %77

; <label>:77:                                     ; preds = %69, %61
  %78 = phi float [ %68, %61 ], [ %76, %69 ]
  store float %78, float* %21, align 4
  %79 = load float, float* %20, align 4
  %80 = load float, float* %21, align 4
  %81 = fmul float %79, %80
  %82 = load float**, float*** %9, align 8
  %83 = load i64, i64* %18, align 8
  %84 = getelementptr inbounds float*, float** %82, i64 %83
  %85 = load float*, float** %84, align 8
  %86 = load i64, i64* %19, align 8
  %87 = getelementptr inbounds float, float* %85, i64 %86
  %88 = load float, float* %87, align 4
  %89 = fadd float %88, %81
  store float %89, float* %87, align 4
  br label %90

; <label>:90:                                     ; preds = %77
  %91 = load i64, i64* %19, align 8
  %92 = add i64 %91, 1
  store i64 %92, i64* %19, align 8
  br label %34

; <label>:93:                                     ; preds = %34
  br label %94

; <label>:94:                                     ; preds = %93
  %95 = load i64, i64* %18, align 8
  %96 = add i64 %95, 1
  store i64 %96, i64* %18, align 8
  br label %29

; <label>:97:                                     ; preds = %29
  br label %98

; <label>:98:                                     ; preds = %97
  %99 = load i64, i64* %17, align 8
  %100 = add i64 %99, 1
  store i64 %100, i64* %17, align 8
  br label %24

; <label>:101:                                    ; preds = %24
  ret void
}

; Function Attrs: noinline nounwind ssp uwtable
define void @dgemm(double**, double**, double**, i64, i64, i64, i1 zeroext, i1 zeroext) #0 {
  %9 = alloca double**, align 8
  %10 = alloca double**, align 8
  %11 = alloca double**, align 8
  %12 = alloca i64, align 8
  %13 = alloca i64, align 8
  %14 = alloca i64, align 8
  %15 = alloca i8, align 1
  %16 = alloca i8, align 1
  %17 = alloca i64, align 8
  %18 = alloca i64, align 8
  %19 = alloca i64, align 8
  %20 = alloca double, align 8
  %21 = alloca double, align 8
  store double** %0, double*** %9, align 8
  store double** %1, double*** %10, align 8
  store double** %2, double*** %11, align 8
  store i64 %3, i64* %12, align 8
  store i64 %4, i64* %13, align 8
  store i64 %5, i64* %14, align 8
  %22 = zext i1 %6 to i8
  store i8 %22, i8* %15, align 1
  %23 = zext i1 %7 to i8
  store i8 %23, i8* %16, align 1
  store i64 0, i64* %17, align 8
  br label %24

; <label>:24:                                     ; preds = %98, %8
  %25 = load i64, i64* %17, align 8
  %26 = load i64, i64* %14, align 8
  %27 = icmp ult i64 %25, %26
  br i1 %27, label %28, label %101

; <label>:28:                                     ; preds = %24
  store i64 0, i64* %18, align 8
  br label %29

; <label>:29:                                     ; preds = %94, %28
  %30 = load i64, i64* %18, align 8
  %31 = load i64, i64* %12, align 8
  %32 = icmp ult i64 %30, %31
  br i1 %32, label %33, label %97

; <label>:33:                                     ; preds = %29
  store i64 0, i64* %19, align 8
  br label %34

; <label>:34:                                     ; preds = %90, %33
  %35 = load i64, i64* %19, align 8
  %36 = load i64, i64* %13, align 8
  %37 = icmp ult i64 %35, %36
  br i1 %37, label %38, label %93

; <label>:38:                                     ; preds = %34
  %39 = load i8, i8* %15, align 1
  %40 = trunc i8 %39 to i1
  br i1 %40, label %41, label %49

; <label>:41:                                     ; preds = %38
  %42 = load double**, double*** %10, align 8
  %43 = load i64, i64* %18, align 8
  %44 = getelementptr inbounds double*, double** %42, i64 %43
  %45 = load double*, double** %44, align 8
  %46 = load i64, i64* %17, align 8
  %47 = getelementptr inbounds double, double* %45, i64 %46
  %48 = load double, double* %47, align 8
  br label %57

; <label>:49:                                     ; preds = %38
  %50 = load double**, double*** %10, align 8
  %51 = load i64, i64* %17, align 8
  %52 = getelementptr inbounds double*, double** %50, i64 %51
  %53 = load double*, double** %52, align 8
  %54 = load i64, i64* %18, align 8
  %55 = getelementptr inbounds double, double* %53, i64 %54
  %56 = load double, double* %55, align 8
  br label %57

; <label>:57:                                     ; preds = %49, %41
  %58 = phi double [ %48, %41 ], [ %56, %49 ]
  store double %58, double* %20, align 8
  %59 = load i8, i8* %16, align 1
  %60 = trunc i8 %59 to i1
  br i1 %60, label %61, label %69

; <label>:61:                                     ; preds = %57
  %62 = load double**, double*** %11, align 8
  %63 = load i64, i64* %17, align 8
  %64 = getelementptr inbounds double*, double** %62, i64 %63
  %65 = load double*, double** %64, align 8
  %66 = load i64, i64* %19, align 8
  %67 = getelementptr inbounds double, double* %65, i64 %66
  %68 = load double, double* %67, align 8
  br label %77

; <label>:69:                                     ; preds = %57
  %70 = load double**, double*** %11, align 8
  %71 = load i64, i64* %19, align 8
  %72 = getelementptr inbounds double*, double** %70, i64 %71
  %73 = load double*, double** %72, align 8
  %74 = load i64, i64* %17, align 8
  %75 = getelementptr inbounds double, double* %73, i64 %74
  %76 = load double, double* %75, align 8
  br label %77

; <label>:77:                                     ; preds = %69, %61
  %78 = phi double [ %68, %61 ], [ %76, %69 ]
  store double %78, double* %21, align 8
  %79 = load double, double* %20, align 8
  %80 = load double, double* %21, align 8
  %81 = fmul double %79, %80
  %82 = load double**, double*** %9, align 8
  %83 = load i64, i64* %18, align 8
  %84 = getelementptr inbounds double*, double** %82, i64 %83
  %85 = load double*, double** %84, align 8
  %86 = load i64, i64* %19, align 8
  %87 = getelementptr inbounds double, double* %85, i64 %86
  %88 = load double, double* %87, align 8
  %89 = fadd double %88, %81
  store double %89, double* %87, align 8
  br label %90

; <label>:90:                                     ; preds = %77
  %91 = load i64, i64* %19, align 8
  %92 = add i64 %91, 1
  store i64 %92, i64* %19, align 8
  br label %34

; <label>:93:                                     ; preds = %34
  br label %94

; <label>:94:                                     ; preds = %93
  %95 = load i64, i64* %18, align 8
  %96 = add i64 %95, 1
  store i64 %96, i64* %18, align 8
  br label %29

; <label>:97:                                     ; preds = %29
  br label %98

; <label>:98:                                     ; preds = %97
  %99 = load i64, i64* %17, align 8
  %100 = add i64 %99, 1
  store i64 %100, i64* %17, align 8
  br label %24

; <label>:101:                                    ; preds = %24
  ret void
}

attributes #0 = { noinline nounwind ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"Apple LLVM version 9.0.0 (clang-900.0.39.2)"}
