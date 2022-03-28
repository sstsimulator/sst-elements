; ModuleID = 'cdfg_memory-01.cpp'
source_filename = "cdfg_memory-01.cpp"

define dso_local void @_Z17offload_loop_testPd(double* %c) #4 {
entry:
  %c.addr = alloca double*, align 8
  %x = alloca double, align 8
  %y = alloca double, align 8
  %z = alloca double, align 8
  store double* %c, double** %c.addr, align 8
  %0 = load double, double* %z, align 8
  %call = call double @cos(double %0) #3
  %call1 = call double @sin(double %call) #3
  %1 = load double, double* %x, align 8
  %mul = fmul double 3.000000e+00, %1
  %2 = load double, double* %y, align 8
  %mul2 = fmul double %mul, %2
  %add = fadd double %call1, %mul2
  %3 = load double*, double** %c.addr, align 8
  store double %add, double* %3, align 8
  ret void
}

; Function Attrs: nounwind
declare dso_local double @sin(double) #2

; Function Attrs: nounwind
declare dso_local double @cos(double) #2

