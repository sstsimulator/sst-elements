# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os.path
import re
import filecmp
import shutil

################################################################################
# Tests related to memory including:
#   - Backends
#   - Backing store
#   - Custom memory instructions
################################################################################

class testcase_memHierarchy_memory(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    # Test writing memory backing to mmap
    # Pass if output mmap file matches ref file
    def test_memory_backing_0_mmap_out(self):
        out_file = "{}/test_memHierarchy_memory_backing_0_mmap_out.mmap.mem".format(self.get_test_output_run_dir())
        ref_file = "{}/refFiles/test_memHierarchy_memory_backing_out.mmap.mem".format(self.get_testsuite_dir())
        self.memh_template_backing(teststr="mmap_out", testnum=0, seed0=0, seed1=1, seed2=2, seed3=3, backing_infile=None, backing_outfile=out_file, backing_reffile=ref_file)
    
    # Test read mmap backing - different files
    # Pass if output mmap file matches input file
    def test_memory_backing_1_mmap_in(self):
        # Need to copybacking_reffile to create "infile" -> copy to $output/tmp_data
        ref_file = "{}/refFiles/test_memHierarchy_memory_backing_out.mmap.mem".format(self.get_testsuite_dir())
        in_file =  "{}/test_memHierarchy_memory_backing_1_mmap_in.mmap.mem".format(self.get_test_output_tmp_dir())
        out_file = "{}/test_memHierarchy_memory_backing_1_mmap_in.mmap.mem".format(self.get_test_output_run_dir())
        shutil.copyfile(ref_file, in_file)
       
        self.memh_template_backing(teststr="mmap_in", testnum=1, seed0=4, seed1=5, seed2=6, seed3=7, backing_infile=in_file, backing_outfile=out_file, backing_reffile=ref_file)
    
    # Test read & write mmap backing - same file
    # Pass if output mmap file matches ref file
    def test_memory_backing_2_mmap_inout(self):
        # Need to copybacking_reffile tobacking_outfile
        ref_file = "{}/refFiles/test_memHierarchy_memory_backing_2_mmap_inout.mmap.mem".format(self.get_testsuite_dir())
        inout_src_file = "{}/refFiles/test_memHierarchy_memory_backing_out.mmap.mem".format(self.get_testsuite_dir())
        inout_file = "{}/test_memHierarchy_memory_backing_2_mmap_inout.mmap.mem".format(self.get_test_output_run_dir())
        shutil.copyfile(ref_file, inout_file)
        
        self.memh_template_backing(teststr="mmap_inout", testnum=2, seed0=8, seed1=9, seed2=10, seed3=11, backing_infile=inout_file, backing_outfile=inout_file, backing_reffile=ref_file)
    
    # Test write malloc backing
    # Pass if output malloc file matches ref file 
    def test_memory_backing_3_malloc_out(self):
        ref_file = "{}/refFiles/test_memHierarchy_memory_backing_out.malloc.mem".format(self.get_testsuite_dir())
        out_file = "{}/test_memHierarchy_memory_backing_3_malloc_out.malloc.mem".format(self.get_test_output_run_dir())
        self.memh_template_backing(teststr="malloc_out", testnum=3, seed0=12, seed1=13, seed2=14, seed3=15, backing_infile=None, backing_outfile=out_file, backing_reffile=ref_file)
    
    # Test read malloc backing
    # Pass if output malloc file matches input file
    def test_memory_backing_4_malloc_in(self):
        ref_file = "{}/refFiles/test_memHierarchy_memory_backing_out.malloc.mem".format(self.get_testsuite_dir())
        in_file = "{}/test_memHierarchy_memory_backing_4_malloc_in.malloc.mem".format(self.get_testsuite_dir())
        out_file = "{}/test_memHierarchy_memory_backing_4_malloc_in.malloc.mem".format(self.get_test_output_run_dir())
        shutil.copyfile(ref_file, in_file) 
        
        self.memh_template_backing(teststr="malloc_in", testnum=4, seed0=16, seed1=17, seed2=18, seed3=19, backing_infile=in_file, backing_outfile=out_file, backing_reffile=ref_file)

    # Test initialization of memory backing from cores during init()
    # Pass if output (malloc) file matches ref file
    def test_memory_backing_5_init(self):
        ref_file = "{}/refFiles/test_memHierarchy_memory_backing_5_init.malloc.mem".format(self.get_testsuite_dir())
        out_file = "{}/test_memHierarchy_memory_backing_5_init.malloc.mem".format(self.get_test_output_run_dir())
        
        self.memh_template_backing(teststr="init", testnum=5, seed0=20, seed1=21, seed2=22, seed3=23, backing_infile=None, backing_reffile=ref_file, backing_outfile=out_file)


#####

    def memh_template_backing(self, teststr, testnum, seed0, seed1, seed2, seed3, backing_infile=None, backing_reffile=None, backing_outfile=None, ignore_err_file=False, testtimeout=240):
         
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        test_run_dir = self.get_test_output_run_dir()

        test_config = "{}/test_backing.py".format(test_path)
        test_output = "{}/test_memHierarchy_memory_backing_{}_{}.out".format(test_run_dir, testnum, teststr)
        test_err    = "{}/test_memHierarchy_memory_backing_{}_{}.err".format(test_run_dir, testnum, teststr)
        test_mpi_output = "{}/test_memHierarchy_memory_backing_{}_{}.testfile".format(self.get_test_output_run_dir(), testnum, teststr)
        
        # Create test arguments
        args = '--model-options="{} {} {} {} {} {} {}"'.format(testnum, seed0, seed1, seed2, seed3, backing_infile, backing_outfile)

        ## Output only in debug mode
        log_debug("testcase = test_backing_{}_{}".format(testnum, teststr))
        log_debug("sdl file = {}".format(test_config))
        log_debug("ref file = {}".format(backing_reffile))

        # Run SST in the tests directory
        self.run_sst(test_config, test_output, test_err, other_args=args, set_cwd=test_path,
                     timeout_sec=testtimeout, mpi_out_files=test_mpi_output)
        
        # Check that simulation completed
        with open(test_output) as fn:
            self.assertIn("Simulation is complete", fn.read(), "No end of simulation detected in output file {}".format(test_output))

        # Check that memory contents match reference
        memcheck = filecmp.cmp(backing_reffile, backing_outfile)
        self.assertTrue(memcheck, "Output memory contents {} does not pass check against the reference file {} ".format(backing_outfile, backing_reffile))
       
        # Check that the simulation generated no stderr output
        self.assertFalse(os_test_file(test_err, "-s"), "Error file is non-empty {}".format(test_err))



