; ModuleID = 'cdfg_memory-02.cpp'
source_filename = "cdfg_memory-02.cpp"

define dso_local void @_Z17offload_loop_testdPd(double %temp, double* %ret_val) #4 {
entry:
  %temp.addr = alloca double, align 8
  %ret_val.addr = alloca double*, align 8
  %x = alloca double, align 8
  %y = alloca double, align 8
  %z = alloca double, align 8
  %a = alloca double, align 8
  %b = alloca double, align 8
  %c = alloca double, align 8
  store double %temp, double* %temp.addr, align 8
  store double* %ret_val, double** %ret_val.addr, align 8
  %0 = load double, double* %temp.addr, align 8
  %call = call double @sin(double %0) #3
  store double %call, double* %x, align 8
  %1 = load double, double* %x, align 8
  %call1 = call double @tan(double %1) #3
  store double %call1, double* %y, align 8
  %2 = load double, double* %x, align 8
  %3 = load double, double* %y, align 8
  %mul = fmul double %2, %3
  %mul2 = fmul double %mul, 4.000000e+00
  store double %mul2, double* %z, align 8
  %4 = load double, double* %temp.addr, align 8
  %call3 = call double @cos(double %4) #3
  store double %call3, double* %a, align 8
  %5 = load double, double* %temp.addr, align 8
  %call4 = call double @sin(double %5) #3
  store double %call4, double* %b, align 8
  %6 = load double, double* %a, align 8
  %7 = load double, double* %b, align 8
  %add = fadd double %6, %7
  store double %add, double* %c, align 8
  %8 = load double, double* %z, align 8
  %9 = load double*, double** %ret_val.addr, align 8
  store double %8, double* %9, align 8
  ret void
}

; Function Attrs: nounwind
declare dso_local double @sin(double) #2

; Function Attrs: nounwind
declare dso_local double @tan(double) #2

; Function Attrs: nounwind
declare dso_local double @cos(double) #2

