
CXXFLAGS = -I. -Wall -O3 -g -std=c++11
OBJECTS = McNiagara.o McNiagara-sim.o Dependency.o MemoryModel.o CycleTracker.o stringutil.o

mcniagara: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o mcniagara

clean:
	rm *.o *~

dox:
	doxygen doxygen.cfg

McNiagara.o: McNiagara.h OffCpuIF.h inst_prob.data perf_cnt.data mersenne.h 
McNiagara-sim.o: McNiagara.h OffCpuIF.h inst_prob.data perf_cnt.data mersenne.h 
#McNiagara-mem.o: McNiagara.h OffCpuIF.h inst_prob.data perf_cnt.data mersenne.h 
Dependency.o: Dependency.h
MemoryModel.o: MemoryModel.h
CycleTracker.o: CycleTracker.h 
stringutil.o: stringutil.h
