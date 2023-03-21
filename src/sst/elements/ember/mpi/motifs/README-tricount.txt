embertricount.cc is a motif providing a model of a triangle counting workload.
The model is derived from the SHAD/GMT reference implementation used within the iARPA AGILE program.

STEP I: Precompute edge index array

The embertricount_setup executable (installed to $(exec_prefix)/bin) precomputes the "Vertices" array
(index into the distributed edge array for each vertex). For the reference implementation, this is sufficient
information to determine the communication pattern.

The arguments to embertricount_setup are identical to the reference implementation
with an additional filename parameter.
Usage: <seed> <scale> <edge ratio> <A> <B> <C> <filename>

STEP II: Run motif through SST

Adding the following addMotif command to an SST python input will cause the tricount motif to be run
using the vertices4.txt input file and a debug level of 1.
ep.addMotif("TriCount vertices_filename=vertices4.txt debug_level=1")

A sample input can be found in sst-elements/src/sst/elements/ember/test/tricount.py
