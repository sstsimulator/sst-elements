; ModuleID = 'cdfg_memory-03.cpp'
source_filename = "cdfg_memory-03.cpp"

define dso_local void @_Z17offload_loop_testPdS_(double* %x, double* %z) #4 {
entry:
  %x.addr = alloca double*, align 8
  %z.addr = alloca double*, align 8
  %a = alloca double, align 8
  %b = alloca double, align 8
  %c = alloca double, align 8
  %d = alloca double, align 8
  %e = alloca double, align 8
  %f = alloca double, align 8
  store double* %x, double** %x.addr, align 8
  store double* %z, double** %z.addr, align 8
  store double 3.140000e+00, double* %a, align 8
  store double 4.200000e+01, double* %b, align 8
  %0 = load double, double* %a, align 8
  %add = fadd double %0, 1.007850e+02
  store double %add, double* %c, align 8
  %1 = load double, double* %b, align 8
  %sub = fsub double %1, 5.002130e+02
  store double %sub, double* %d, align 8
  %2 = load double, double* %c, align 8
  %3 = load double, double* %d, align 8
  %div = fdiv double %2, %3
  store double %div, double* %e, align 8
  %4 = load double, double* %d, align 8
  %5 = load double, double* %c, align 8
  %mul = fmul double %4, %5
  store double %mul, double* %f, align 8
  %6 = load double, double* %e, align 8
  %7 = load double*, double** %x.addr, align 8
  store double %6, double* %7, align 8
  %8 = load double, double* %f, align 8
  %9 = load double*, double** %z.addr, align 8
  store double %8, double* %9, align 8
  ret void
}

