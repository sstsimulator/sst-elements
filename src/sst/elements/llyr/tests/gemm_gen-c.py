#!/usr/bin/python

import itertools
from collections import defaultdict

debug = 0

#m = 2
#k = 1
#n = 1

#m = 2
#k = 4
#n = 2

m = 2
k = 3
n = 2

#m = 6
#k = 3
#n = 10

num_load = (m * k) + (k * n)
num_mul = k * m * n
num_add = (k - 1) * (m * n)
num_store = m * n

pe_string_pre = "tempPE = new"
pe_string_app = "llyr_config"

vertex_string_pre = "graphOut.addVertex"
vertex_string_app = "tempPE"

edge_string_pre = "graphOut.addEdge"

##
def extract_groups( grouping ):
    temp_list = list(itertools.zip_longest(*(iter(grouping),) * 2, fillvalue=-1))
    return temp_list

if( debug == 1 ):
    print("Num load: " + str(num_load) + " 1-" + str(num_load))
    print("Num mult: " + str(num_mul) + " " + str(1 + num_load) + "-" + str(num_load+num_mul))
    print("Num add: " + str(num_add) + " " + str(1 + num_mul + num_load) + "-" + str(num_load+num_mul+num_add))
    print("Num store: " + str(num_store) + " " + str(1 + num_add + num_mul + num_load) + "-" + str(num_add+num_load+num_mul+num_store))
    print("\n")


# open the file
file = open("gemm.out", "w")

#create dummy
file.write("ProcessingElement* tempPE = new DummyProcessingElement(DUMMY, 0, llyr_config);\n")
file.write("graphOut.addVertex( 0, tempPE );\n")
file.write("\n")

pe_num = 1
def write_pe( pe_string, pe, num_pes ):
    file.write("%s %s( %s, %s, %s ); \n" % (pe_string_pre, pe_string, pe, pe_num, pe_string_app))
    file.write("%s( %s, %s ); \n" % (vertex_string_pre, pe_num, vertex_string_app))
    file.write("\n")

# write loads
for counter in range( 0, num_load ):
    write_pe( "LoadProcessingElement", "LD", pe_num )
    pe_num = pe_num + 1

# write mul
mul_start = pe_num
for counter in range( 0, num_mul ):
    write_pe( "IntProcessingElement", "MUL", pe_num )
    pe_num = pe_num + 1

# write add
add_start = pe_num
for counter in range( 0, num_add ):
    write_pe( "IntProcessingElement", "ADD", pe_num )
    pe_num = pe_num + 1

# write store
store_start = pe_num
for counter in range( 0, num_store ):
    write_pe( "StoreProcessingElement", "ST", pe_num )
    pe_num = pe_num + 1

if( debug == 1 ):
    print(" %s %s %s" % (mul_start, add_start, store_start))

# write edges
# first edges are from dummy (node 0) to all of the loads
for counter in range( 1, num_load + 1 ):
    file.write("%s( 0, %s ); \n" % (edge_string_pre, counter))
file.write("\n")

# connect loads to muls (first load is PE-1)
a_start = 1
a_end = (k * m)
a_next = a_start
b_start = a_end + 1
b_end = b_start + ((k * n) - 1)
b_next = b_start
if( debug == 1 ):
    print("LD-MUL  A:%s-%s B:%s-%s\n" % (a_start, a_end, b_start, b_end))
    file.write("LD-MUL  A:%s-%s B:%s-%s\n" % (a_start, a_end, b_start, b_end))

mul_pe_dict = defaultdict(list)
add_pe_list = []
final_add_pe_list = []
new_mul = mul_start
for x in range( 0, m ):
    b_offset = 0
    a_offset = a_start + (x * k)
    mul_pe = new_mul
    for y in range( 0, n ):
        temp_list = []
        b_next = b_start + b_offset
        #file.write("PE Start " + str(mul_pe) + " B-" + str(b_next) + "\n")
        for z in range( 0, k ):
            a_next = a_offset + z
            mul_pe_dict[a_next].append(mul_pe)
            mul_pe_dict[b_next].append(mul_pe)
            temp_list.append(mul_pe)
            #file.write("%s( %s, %s ); \n" % (edge_string_pre, a_next, b_next))
            b_next = b_next + n
            mul_pe = mul_pe + 1
        #for key in mul_pe_dict.keys():
            #val = mul_pe_dict[key]
            #print("Key", key, 'points to', val)
        #print("\n")
        #file.write("\n")
        add_pe_list.append(temp_list)
        b_offset =  b_offset + 1
    new_mul = mul_pe

if( debug == 1 ):
    for val in add_pe_list:
        print(val)
    print("\n")

for key in mul_pe_dict.keys():
    value = mul_pe_dict[key]
    for val in value:
        file.write("%s( %s, %s ); \n" % (edge_string_pre, key, val))
    file.write("\n")

# connect adds to muls
test = []
if( debug == 1 ):
    file.write("MUL-ADD  A:%s-%s B:%s-%s\n" % (a_start, a_end, b_start, b_end))

def add_tree( boop, next_add ):
    global add_start
    add_pe_groups = []
    if( len(boop) > 1 ):
        if( debug == 1 ):
            print("Here")
            print(boop)
        for x in boop:
            if( x.count(-1) == 0 ):
                file.write("%s( %s, %s ); \n" % (edge_string_pre, x[0], next_add))
                file.write("%s( %s, %s ); \n" % (edge_string_pre, x[1], next_add))
                add_pe_groups.append(next_add)
                next_add = next_add + 1
                if( debug == 1 ):
                    print(x)
            else:
                add_pe_groups.append(x[0])
                if( debug == 1 ):
                    print(x)

        add_start = next_add
        if( debug == 1 ):
            print(add_pe_groups)
        test = extract_groups(add_pe_groups)
        if( debug == 1 ):
            print(test)
        add_tree(test, add_start)
    else:
        if( debug == 1 ):
            print("Here 2")
            print(boop)
        for x in boop:
            if( x.count(-1) == 0 ):
                file.write("%s( %s, %s ); \n" % (edge_string_pre, x[0], next_add))
                file.write("%s( %s, %s ); \n" % (edge_string_pre, x[1], next_add))
                add_pe_groups.append(next_add)
                final_add_pe_list.append(next_add)
                next_add = next_add + 1
                if( debug == 1 ):
                    print(x)
            else:
                final_add_pe_list.append(x[0])
                if( debug == 1 ):
                    print(x)

        add_start = next_add
        if( debug == 1 ):
            print(add_pe_groups)
            print(final_add_pe_list)

for value in add_pe_list:
    test = list(itertools.zip_longest(*(iter(value),) * 2, fillvalue=-1))
    if( debug == 1 ):
        print("ADD")
        print(test)
    add_tree(test, add_start)
    file.write("\n")
    if( debug == 1 ):
        print("\n")

## Connect stores
next_store = store_start
for value in final_add_pe_list:
    if( debug == 1 ):
        print("ST")
        print(value)
    file.write("%s( %s, %s ); \n" % (edge_string_pre, value, next_store))
    next_store = next_store + 1

file.write("\n")


# cleanup
file.close()
