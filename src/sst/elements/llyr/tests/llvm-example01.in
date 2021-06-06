; ModuleID = 'cdfg_example-01.ll'
source_filename = "cdfg_example-01.cpp"

define dso_local double @_Z17offload_loop_testv() {
entry:
  %x = alloca double, align 8
  %y = alloca double, align 8
  %z = alloca double, align 8
  %0 = load double, double* %z, align 8
  %call = call double @cos(double %0)
  %call1 = call double @sin(double %call)
  %1 = load double, double* %x, align 8
  %mul = fmul double 3.000000e+00, %1
  %2 = load double, double* %y, align 8
  %mul2 = fmul double %mul, %2
  %add = fadd double %call1, %mul2
  ret double %add
}

declare dso_local double @sin(double)

declare dso_local double @cos(double)
