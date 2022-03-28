; ModuleID = 'cdfg_simple-01.cpp'
source_filename = "cdfg_simple-01.cpp"

define dso_local void @_Z17offload_loop_testPd(double* %x) #4 {
entry:
  %x.addr = alloca double*, align 8
  %a = alloca double, align 8
  %b = alloca double, align 8
  store double* %x, double** %x.addr, align 8
  store double 3.140000e+00, double* %a, align 8
  store double 4.200000e+01, double* %b, align 8
  %0 = load double, double* %a, align 8
  %1 = load double, double* %b, align 8
  %add = fadd double %0, %1
  %2 = load double*, double** %x.addr, align 8
  store double %add, double* %2, align 8
  ret void
}


