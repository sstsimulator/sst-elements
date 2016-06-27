
#
# Uncomment to use the DUMA memory debugging library (and comment the blank ones)
#
#MEMDEBUGI = -DMEMDEBUG -I/home/joncook/tools/duma_2_5_15
#MEMDEBUGL = -L/home/joncook/tools/duma_2_5_15 -lduma -lpthread
MEMDEBUGI = 
MEMDEBUGL = 

# -pg -g
CXXFLAGS = -I. -Wall -O3 $(MEMDEBUGI)
LDFLAGS = -g
OBJECTS = FunctionalUnit.o InstructionQueue.o McOpteron.o Dependency.o MarkovModel.o Token.o InstructionInfo.o \
          Random.o LoadStoreUnit.o MemoryModel.o ReorderBuffer.o ConfigVars.o OpteronDefs.o

mcopteron: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o mcopteron $(MEMDEBUGL)

clean:
	rm -f *.o *~

dox:
	doxygen doxygen.cfg

