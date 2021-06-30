#!/usr/bin/python

import itertools
from collections import defaultdict

debug = 0
data_type = 0   #0 int, 1 fp64

## mxn * nxk
#m = 1
#n = 3
#k = 2
m = 3
n = 3
k = 2
#m = 3
#n = 3
#k = 3

## init non-zeroes
#non_zeroes = [1]
non_zeroes = [1,2,3,4,5]

## init row_ptr
#row_ptr = [0,1]
#row_ptr = [0,1,3,5]
row_ptr = [0,1,4,5]

## init col_ptr
#col_ptr = [0,1]
#col_ptr = [0,0,1,1,2]
col_ptr = [0,0,1,2,2]

## init dense mat
dense_mat = []
for i in range( n ):
   x = []
   for j in range( k ):
      x.append(0)
   dense_mat.append(x)

dense_mat[0][0] = 6
dense_mat[0][1] = 7
dense_mat[1][0] = 8
dense_mat[1][1] = 9
dense_mat[2][0] = 10
dense_mat[2][1] = 11

#dense_mat[0][2] = 12
#dense_mat[1][2] = 13
#dense_mat[2][2] = 14


## write memory file
mem_file = open("spmm-mem.in", "w")

# write k
mem_file.write(str(k) + ",")

#write v
converted_list = [str(element) for element in non_zeroes]
mem_file.write(",".join(converted_list))
mem_file.write(",")

#write row_ptr
converted_list = [str(element) for element in row_ptr]
mem_file.write(",".join(converted_list))
mem_file.write(",")

#write col_ptr
converted_list = [str(element) for element in col_ptr]
mem_file.write(",".join(converted_list))
mem_file.write(",")

for i in dense_mat:
   converted_list = [str(element) for element in i]
   mem_file.write(",".join(converted_list))
   mem_file.write(",")

#fill memory
for i in range(255):
   mem_file.write(str(255))
   if( i != 255 ):
      mem_file.write(",")

mem_file.close()

#####################################
pe_string_pre = " [pe_type="
adj_list = defaultdict(list)

result = []
for i in range( m ):
   x = []
   for j in range( k ):
      x.append(0)
   result.append(x)

# open the file
file = open("spmm.in", "w")

#for x in result:
   #converted_list = [str(element) for element in x]
   #print(",".join(converted_list))

v_start_addr = 8
r_start_addr = v_start_addr + (8 * len(non_zeroes))
c_start_addr = r_start_addr + (8 * len(row_ptr))
d_start_addr = c_start_addr + (8 * len(col_ptr))
store_start_addr = 256

print("size of non_zeroes = " + str(len(non_zeroes)) + " " + str(v_start_addr))
print("size of row_ptr = " + str(len(row_ptr)) + " " + str(r_start_addr))
print("size of col_ptr = " + str(len(col_ptr)) + " " + str(c_start_addr))
print("size of dense = " + str(len(dense_mat)) + " " + str(d_start_addr))

# write ld for k
adj_list[1].append(2)
file.write( str(1) + pe_string_pre + "LDADDR,0" + "]" + "\n" )
file.write( str(2) + pe_string_pre + "MULCONST,8" + "]" + "\n" )
file.write( "\n" )

pe_id = 3
for x in range( 0, m ):
   sp_ld_dict = defaultdict(int)
   col_ld_dict = defaultdict(int)
   partial_product = defaultdict(list)

   for y in range( row_ptr[x], row_ptr[x+1] ):
      ## LD sparse[j]
      temp_addr = v_start_addr + (8 * y)
      file.write( str(pe_id) + pe_string_pre + "LDADDR," + str(temp_addr) + "]" + "\n" )
      sp_ld_dict[y] = pe_id
      pe_id = pe_id + 1

      ## LD column[j]
      temp_addr = c_start_addr + (8 * y)
      file.write( str(pe_id) + pe_string_pre + "LDADDR," + str(temp_addr) + "]" + "\n" )
      col_ld_dict[y]= pe_id
      pe_id = pe_id + 1

      for z in range( 0, k ):
         #print( str(x) + " " + str(y) + " " + str(z) )
         #print( str(result[x][z]) + " + " + str (non_zeroes[y]) + " * " + str(dense_mat[col_ptr[y]][z]) )
         result[x][z] = result[x][z] + non_zeroes[y] * dense_mat[col_ptr[y]][z]

         # calculate col in d[][]
         mul_1 = pe_id
         temp_pe = col_ld_dict[y]
         adj_list[temp_pe].append(pe_id)
         adj_list[2].append(pe_id)
         file.write(str(pe_id) + pe_string_pre + "MUL" + "]" + "\n")
         pe_id = pe_id + 1

         # calculate row in d[][]
         offset_pe = pe_id
         adj_list[mul_1].append(pe_id)
         offset = d_start_addr + (z * 8)
         file.write(str(pe_id) + pe_string_pre + "ADDCONST," + str(offset) + "]" + "\n")
         pe_id = pe_id + 1

         # sparse[j] * dense[]
         mul_2 = pe_id
         temp_pe = sp_ld_dict[y]
         adj_list[temp_pe].append(pe_id)
         partial_product[z].append(mul_2)
         file.write(str(pe_id) + pe_string_pre + "MUL" + "]" + "\n")
         pe_id = pe_id + 1

         # LD dense[col[j]][k]
         ld_dense = pe_id
         adj_list[offset_pe].append(pe_id)
         adj_list[pe_id].append(mul_2)
         file.write(str(pe_id) + pe_string_pre + "LD" + "]" + "\n")
         pe_id = pe_id + 1

         ## ST final value
         #final_st[z].append(pe_id)
         #file.write(str(pe_id) + pe_string_pre + "STADDR," + str(store_start_addr) + "]" + "\n")
         #store_start_addr = store_start_addr + 8
         #pe_id = pe_id + 1

         file.write( "\n" )

   #for key in sp_ld_dict.keys():
      #val = sp_ld_dict[key]
      #print("SP Key", key, 'points to', val)

   #for key in col_ld_dict.keys():
      #val = col_ld_dict[key]
      #print("COL Key", key, 'points to', val)

   for key in partial_product.keys():
      val = partial_product[key]
      #print("PP Key", key, 'points to', val)

      # match the accumulators
      if( len(val) > 1 ):
         acc_list = []
         #print("Moo! Cows! " + str(len(val)))
         while( len(val) > 1 ):
            #print("A " + str(len(val)))
            adj_list[val.pop()].append(pe_id)
            adj_list[val.pop()].append(pe_id)
            acc_list.append(pe_id)
            file.write(str(pe_id) + pe_string_pre + "ADD" + "]" + "\n")
            pe_id = pe_id + 1
         if( len(val) > 0 ):
            #print("B " + str(len(val)))
            adj_list[val.pop()].append(pe_id)
            acc_list.append(pe_id)
            file.write(str(pe_id) + pe_string_pre + "ADD" + "]" + "\n")
            pe_id = pe_id + 1
         # adder tree
         if( len(acc_list) > 1 ):
            #print("Boo! Who? " + str(len(acc_list)))
            while( len(acc_list) > 1 ):
               #print("C " + str(len(acc_list)))
               # check if the adder has a single input
               num_found = 0
               for key in adj_list.keys():
                  top = adj_list[key]
                  if( acc_list[-1] in top ):
                     num_found = num_found + 1
                     #print("Found " + str(top))

               # if the tail has a single input, it's the bottom of the tree because muls are grouped above
               if( num_found == 1 ):
                  adj_list[acc_list[-2]].append(acc_list[-1])

               # otherwise we need another adder
               else:
                  adj_list[acc_list[-1]].append(pe_id)
                  adj_list[acc_list[-2]].append(pe_id)
                  file.write(str(pe_id) + pe_string_pre + "ADD" + "]" + "\n")
                  pe_id = pe_id + 1

               #print(str(acc_list.pop()))
               #print(str(acc_list.pop()))
               acc_list.pop()
               acc_list.pop()

            if( len(acc_list) > 0 ):
               #print("D " + str(len(acc_list)))
               adj_list[pe_id - 1].append(pe_id)
               file.write(str(pe_id) + pe_string_pre + "ADD" + "]" + "\n")
               pe_id = pe_id + 1
               print(str(acc_list.pop()))

            # ST final value
            adj_list[pe_id - 1].append(pe_id)
            file.write(str(pe_id) + pe_string_pre + "STADDR," + str(store_start_addr) + "]" + "\n")
            store_start_addr = store_start_addr + 8
            pe_id = pe_id + 1

         else:
            #print("Boo! " + str(acc_list[0]))

            # ST final value
            file.write(str(pe_id) + pe_string_pre + "STADDR," + str(store_start_addr) + "]" + "\n")
            store_start_addr = store_start_addr + 8
            pe_id = pe_id + 1

      else:
         #print("Narf! " + str(partial_product[key][0]))
         # ST final value
         adj_list[partial_product[key][0]].append(pe_id)
         file.write(str(pe_id) + pe_string_pre + "STADDR," + str(store_start_addr) + "]" + "\n")
         store_start_addr = store_start_addr + 8
         pe_id = pe_id + 1

   #print( "\n" )

for key in adj_list.keys():
   val = adj_list[key]
   #print("ADJ Key", key, 'points to', val)
   for i in val:
      file.write(str(key) + " -- " + str(i) + "\n")
   file.write("\n")

for x in result:
   converted_list = [str(element) for element in x]
   print(",".join(converted_list))



# cleanup
file.close()

