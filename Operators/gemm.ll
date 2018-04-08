; ModuleID = '/Users/Aman/code/Hobbit/Operators/gemm.c'
source_filename = "/Users/Aman/code/Hobbit/Operators/gemm.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.12.0"

; Function Attrs: nounwind ssp uwtable
define void @sgemm([1024 x float]*, [1024 x float]*, [1024 x float]*) #0 {
  %4 = alloca [1024 x float]*, align 8
  %5 = alloca [1024 x float]*, align 8
  %6 = alloca [1024 x float]*, align 8
  %7 = alloca i32, align 4
  %8 = alloca i32
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  store [1024 x float]* %0, [1024 x float]** %4, align 8, !tbaa !3
  store [1024 x float]* %1, [1024 x float]** %5, align 8, !tbaa !3
  store [1024 x float]* %2, [1024 x float]** %6, align 8, !tbaa !3
  %11 = bitcast i32* %7 to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %11) #2
  store i32 0, i32* %7, align 4, !tbaa !7
  br label %12

; <label>:12:                                     ; preds = %73, %3
  %13 = load i32, i32* %7, align 4, !tbaa !7
  %14 = icmp slt i32 %13, 1024
  br i1 %14, label %17, label %15

; <label>:15:                                     ; preds = %12
  store i32 2, i32* %8, align 4
  %16 = bitcast i32* %7 to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %16) #2
  br label %76

; <label>:17:                                     ; preds = %12
  %18 = bitcast i32* %9 to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %18) #2
  store i32 0, i32* %9, align 4, !tbaa !7
  br label %19

; <label>:19:                                     ; preds = %69, %17
  %20 = load i32, i32* %9, align 4, !tbaa !7
  %21 = icmp slt i32 %20, 1024
  br i1 %21, label %24, label %22

; <label>:22:                                     ; preds = %19
  store i32 5, i32* %8, align 4
  %23 = bitcast i32* %9 to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %23) #2
  br label %72

; <label>:24:                                     ; preds = %19
  %25 = bitcast i32* %10 to i8*
  call void @llvm.lifetime.start.p0i8(i64 4, i8* %25) #2
  store i32 0, i32* %10, align 4, !tbaa !7
  br label %26

; <label>:26:                                     ; preds = %65, %24
  %27 = load i32, i32* %10, align 4, !tbaa !7
  %28 = icmp slt i32 %27, 1024
  br i1 %28, label %31, label %29

; <label>:29:                                     ; preds = %26
  store i32 8, i32* %8, align 4
  %30 = bitcast i32* %10 to i8*
  call void @llvm.lifetime.end.p0i8(i64 4, i8* %30) #2
  br label %68

; <label>:31:                                     ; preds = %26
  %32 = load [1024 x float]*, [1024 x float]** %4, align 8, !tbaa !3
  %33 = load i32, i32* %7, align 4, !tbaa !7
  %34 = sext i32 %33 to i64
  %35 = getelementptr inbounds [1024 x float], [1024 x float]* %32, i64 %34
  %36 = load i32, i32* %9, align 4, !tbaa !7
  %37 = sext i32 %36 to i64
  %38 = getelementptr inbounds [1024 x float], [1024 x float]* %35, i64 0, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !9
  %40 = load [1024 x float]*, [1024 x float]** %5, align 8, !tbaa !3
  %41 = load i32, i32* %7, align 4, !tbaa !7
  %42 = sext i32 %41 to i64
  %43 = getelementptr inbounds [1024 x float], [1024 x float]* %40, i64 %42
  %44 = load i32, i32* %10, align 4, !tbaa !7
  %45 = sext i32 %44 to i64
  %46 = getelementptr inbounds [1024 x float], [1024 x float]* %43, i64 0, i64 %45
  %47 = load float, float* %46, align 4, !tbaa !9
  %48 = load [1024 x float]*, [1024 x float]** %6, align 8, !tbaa !3
  %49 = load i32, i32* %10, align 4, !tbaa !7
  %50 = sext i32 %49 to i64
  %51 = getelementptr inbounds [1024 x float], [1024 x float]* %48, i64 %50
  %52 = load i32, i32* %9, align 4, !tbaa !7
  %53 = sext i32 %52 to i64
  %54 = getelementptr inbounds [1024 x float], [1024 x float]* %51, i64 0, i64 %53
  %55 = load float, float* %54, align 4, !tbaa !9
  %56 = fmul float %47, %55
  %57 = fadd float %39, %56
  %58 = load [1024 x float]*, [1024 x float]** %4, align 8, !tbaa !3
  %59 = load i32, i32* %7, align 4, !tbaa !7
  %60 = sext i32 %59 to i64
  %61 = getelementptr inbounds [1024 x float], [1024 x float]* %58, i64 %60
  %62 = load i32, i32* %9, align 4, !tbaa !7
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds [1024 x float], [1024 x float]* %61, i64 0, i64 %63
  store float %57, float* %64, align 4, !tbaa !9
  br label %65

; <label>:65:                                     ; preds = %31
  %66 = load i32, i32* %10, align 4, !tbaa !7
  %67 = add nsw i32 %66, 1
  store i32 %67, i32* %10, align 4, !tbaa !7
  br label %26

; <label>:68:                                     ; preds = %29
  br label %69

; <label>:69:                                     ; preds = %68
  %70 = load i32, i32* %9, align 4, !tbaa !7
  %71 = add nsw i32 %70, 1
  store i32 %71, i32* %9, align 4, !tbaa !7
  br label %19

; <label>:72:                                     ; preds = %22
  br label %73

; <label>:73:                                     ; preds = %72
  %74 = load i32, i32* %7, align 4, !tbaa !7
  %75 = add nsw i32 %74, 1
  store i32 %75, i32* %7, align 4, !tbaa !7
  br label %12

; <label>:76:                                     ; preds = %15
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64, i8* nocapture) #1

attributes #0 = { nounwind ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+fxsr,+mmx,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{!"clang version 6.0.0 (tags/RELEASE_600/final)"}
!3 = !{!4, !4, i64 0}
!4 = !{!"any pointer", !5, i64 0}
!5 = !{!"omnipotent char", !6, i64 0}
!6 = !{!"Simple C/C++ TBAA"}
!7 = !{!8, !8, i64 0}
!8 = !{!"int", !5, i64 0}
!9 = !{!10, !10, i64 0}
!10 = !{!"float", !5, i64 0}
