g++ -c ORION_Util.cc ORION_Router_Info.cc ORION_Array_Info.cc ORION_Crossbar.cc ORION_Array_Internal.cc ORION_Array.cc ORION_Arbiter.cc ORION_Router.cc   
ar rcu liborion.a *.o
ranlib liborion.a
