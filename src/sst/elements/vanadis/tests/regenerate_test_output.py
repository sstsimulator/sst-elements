#!/usr/bin/env python3
import sys
import os
import re
import shutil
import difflib

# Recurse through sst_test_outputs directory
# For every leaf directory, find the corresponding directory in vanadis/tests/ and update the files

if len(sys.argv) < 2:
    print("Error, you must provide the path to the sst_test_outputs directory as an argument")
    sys.exit()

path_output = sys.argv[1]
if path_output[-1] != "/":
    path_output += "/"
path_output += "run_data/vanadis_tests/"

if len(sys.argv) > 2:
    path_ref = sys.argv[2]
    if path_ref[-1] != "/":
        path_ref += "/"
else:
    path_ref = os.getcwd() + "/"

file_list = []

# Patterns to match
sst_stdout = re.compile("test_vanadis_.*.out")

for root, names, files in os.walk(path_output):
    for file in files:
        test_path = root.replace(path_output, path_ref)

        dst_path = ""
        if file == "stdout-100":
            dst_path = os.path.join(test_path, "vanadis.stdout.gold")
        elif file == "stderr-100":
            dst_path = os.path.join(test_path, "vanadis.stderr.gold")
        elif sst_stdout.match(file):
            dst_path = os.path.join(test_path, "sst.stdout.gold")

        src_path = os.path.join(root, file)

        has_diff = False
        if dst_path != "":
            with open(src_path) as src_file:
                src_lines = src_file.readlines()

            with open(dst_path) as dst_file:
                dst_lines = dst_file.readlines()

            has_diff = list(difflib.unified_diff(src_lines, dst_lines, src_path, dst_path, n=1))
        if has_diff:
            print("COPY {} --> {}".format(src_path, dst_path))
            shutil.copy(src_path, dst_path)
