; ModuleID = 'cdfg_memory-01.cpp'
source_filename = "cdfg_memory-01.cpp"

; Function Attrs: noinline nounwind optnone uwtable mustprogress
define dso_local void @_Z17offload_loop_testPm(i64* %c) #4 {
entry:
  %c.addr = alloca i64*, align 8
  %x = alloca i64, align 8
  %y = alloca i64, align 8
  %z = alloca i64, align 8
  store i64* %c, i64** %c.addr, align 8
  %0 = load i64*, i64** %c.addr, align 8
  %1 = load i64, i64* %0, align 8
  store i64 %1, i64* %x, align 8
  %2 = load i64*, i64** %c.addr, align 8
  %3 = load i64, i64* %2, align 8
  store i64 %3, i64* %y, align 8
  %4 = load i64, i64* %x, align 8
  %mul = mul i64 3, %4
  %5 = load i64, i64* %y, align 8
  %mul1 = mul i64 %mul, %5
  %6 = load i64*, i64** %c.addr, align 8
  store i64 %mul1, i64* %6, align 8
  ret void
}

; Function Attrs: nounwind
declare dso_local double @sin(double) #2

; Function Attrs: nounwind
declare dso_local double @cos(double) #2

