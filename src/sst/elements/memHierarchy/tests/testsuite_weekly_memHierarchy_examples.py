# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os.path

# This test suite checks that example configurations shipped with memHierarchy
# will run. It checks for successful run only, there are no reference files.

class testcase_memHierarchy_sdl(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    
    def test_memHierarchy_example_1core_nocache(self):
        self.memHierarchy_example_test("1core-nocache")
    
    def test_memHierarchy_example_1core_1level(self):
        self.memHierarchy_example_test("1core-1level")

    def test_memHierarchy_example_1core_3level(self):
        self.memHierarchy_example_test("1core-3level")

    def test_memHierarchy_example_4core_shared_llc(self):
        self.memHierarchy_example_test("4core-shared-llc")

    def test_memHierarchy_example_4core_shared_distributed_llc(self):
        self.memHierarchy_example_test("4core-shared-distributed-llc")

#####

    def memHierarchy_example_test(self, testcase, ignore_err_file=False):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        example_path = test_path + "/../examples/"
        outdir = self.get_test_output_run_dir()

        # Set the various file paths
        filename=("example-{0}".format(testcase))
        sdlfile = "{0}/{1}.py".format(example_path, filename)
        outfile = "{0}/{1}.out".format(outdir, filename)
        errfile = "{0}/{1}.err".format(outdir, filename)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, filename)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=test_path, mpi_out_files=mpioutfiles)
        
        # Notify is err file is non-empty
        if ignore_err_file is False:
            if os_test_file(errfile, "-s"):
                log_testing_note("memHierarchy example test {0} has a Non-Empty Error File {1}".format(filename, errfile))

        # Check that there is not a FATAL message in the output
        cmd = 'grep "FATAL" {0} '.format(outfile)
        grep_success = os.system(cmd) == 0
        self.assertFalse(grep_success, "Output file {0} contains the word 'FATAL'.".format(outfile))
        
        # Check that there is a successful simulation completion message in the output
        cmd = 'grep "Simulation is complete, simulated time:" {0} '.format(outfile)
        grep_success = os.system(cmd) == 0
        self.assertTrue(grep_success, "Output file {0} does not contain a simulation completion message.".format(outfile))
        
