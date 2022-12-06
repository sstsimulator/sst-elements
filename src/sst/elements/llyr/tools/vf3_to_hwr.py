#!/usr/bin/python3
import sys
import getopt
import numpy as np

input_file = ""
output_file = ""

opts, argv = getopt.getopt(sys.argv[1:], 'i:o:')

for opt, arg in opts:
   if opt in( "-i", "--ifile" ):
      input_file = arg
   elif opt in( "-o", "--ofile" ):
      output_file = arg

first_line = 0
node_list = 0
temp_edges = 0
dotFile = open( output_file, "w" )
parse = open( input_file, 'r' )

dotFile.write( "digraph \"Hardware Description\" {\n" )
#dotFile.write( "layout=sfdp\n" )
#dotFile.write( "node [shape=plaintext]\n" )

for line in parse:
   if( first_line == 0 ):
      first_line = 1
   elif( line == "\n" ):
      if( node_list == 0 ):
         node_list = 1
      elif( node_list == 1 ):
         node_list = 2
         temp_edges = 1
      else:
         temp_edges = 1
   else:
      tempLine = line.strip().split()
      if( node_list == 1 ):
         dotFile.write( str(tempLine[0]) + " " + "[label=any]")
         dotFile.write( "\n" )
      elif( temp_edges == 1 ):
         temp_edges = 0
      else:
         dotFile.write( str(tempLine[0]) + "--" + str(tempLine[1]) )
         dotFile.write( "\n" )

dotFile.write( "}\n" )

parse.close()
dotFile.close()

 #for file in ; do ../convert_dot.py -i $file -o "${file}.dot"; done
 #for file in gemm*; do if [[ $file != *".vf3" ]]; then echo $file; fi done
 #for file in gemm*; do if [[ $file == *".vf3" ]]; then ../convert_dot.py -i $file -o "${file}.dot"; fi done

