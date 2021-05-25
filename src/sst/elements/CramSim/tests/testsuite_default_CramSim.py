# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        try:
            # Put your single instance Init Code Here
            class_inst._setupCramSimTestFiles()
            module_init = 1
        except:
            pass
        module_init = 1

    module_sema.release()

################################################################################

class testcase_CramSim_Component(SSTTestCase):

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        initializeTestModule_SingleInstance(self)

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_CramSim_1_R(self):
        self.CramSim_test_template("1_R")

    def test_CramSim_1_RW(self):
        self.CramSim_test_template("1_RW")

    def test_CramSim_1_W(self):
        self.CramSim_test_template("1_W")

    def test_CramSim_2_R(self):
        self.CramSim_test_template("2_R")

    def test_CramSim_2_W(self):
        self.CramSim_test_template("2_W")

    def test_CramSim_4_R(self):
        self.CramSim_test_template("4_R")

    def test_CramSim_4_W(self):
        self.CramSim_test_template("4_W")

    def test_CramSim_5_R(self):
        self.CramSim_test_template("5_R")

    def test_CramSim_5_W(self):
        self.CramSim_test_template("5_W")

    def test_CramSim_6_R(self):
        self.CramSim_test_template("6_R")

    def test_CramSim_6_W(self):
        self.CramSim_test_template("6_W")

#####

    def CramSim_test_template(self, testcase):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.CramSimElementDir = os.path.abspath("{0}/../".format(test_path))
        self.CramSimElementTestsDir = "{0}/tests".format(self.CramSimElementDir)
        self.testCramSimDir = "{0}/testCramSim".format(tmpdir)
        self.testCramSimTestsDir = "{0}/tests".format(self.testCramSimDir)

        # Set the various file paths
        testDataFileName="test_CramSim_{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        testpyfilepath = "{0}/test_txntrace.py".format(self.testCramSimTestsDir)
        tracefile      = "{0}/sst-CramSim-trace_verimem_{1}.trc".format(self.testCramSimTestsDir, testcase)
        configfile     = "{0}/ddr4_verimem.cfg".format(self.testCramSimDir)

        if os.path.isfile(testpyfilepath):
            sdlfile = testpyfilepath
            otherargs = '--model-options=\"--configfile={0} traceFile={1}\"'.format(configfile, tracefile)
        else:
            sdlfile = "{0}/test_txntrace4.py".format(self.testCramSimTestsDir)
            otherargs = '--model-options=\"--configfile={0} --traceFile={1}\"'.format(configfile, tracefile)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, mpi_out_files=mpioutfiles)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("CramSim test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        # NOTE: This is how the bamboo tests does it, and its very crude.  The
        #       testing_compare_diff will always fail, so all it looks for is the
        #       "Simulation complete" message to decide pass/fail
        #       This should be improved upon to check for real data...
        cmp_result = testing_compare_diff(testDataFileName, outfile, reffile)
        if not cmp_result:
            cmd = 'grep -q "Simulation is complete" {0} '.format(outfile)
            grep_result = os.system(cmd) == 0
            self.assertTrue(grep_result, "Output file {0} does not contain a simulation complete message".format(outfile, reffile))
        else:
            self.assertTrue(cmp_result, "Output file {0} does not match Reference File {1}".format(outfile, reffile))

#####

    def _setupCramSimTestFiles(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setupCramSimTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.CramSimElementDir = os.path.abspath("{0}/../".format(test_path))
        self.CramSimElementTestsDir = "{0}/tests".format(self.CramSimElementDir)
        self.testCramSimDir = "{0}/testCramSim".format(tmpdir)
        self.testCramSimTestsDir = "{0}/tests".format(self.testCramSimDir)

        # Create a clean version of the testCramSim Directory
        if os.path.isdir(self.testCramSimDir):
            shutil.rmtree(self.testCramSimDir, True)
        os.makedirs(self.testCramSimDir)
        os.makedirs(self.testCramSimTestsDir)

        # Create a simlink of the ddr4_verimem.cfg file
        os_symlink_file(self.CramSimElementDir, self.testCramSimDir, "ddr4_verimem.cfg")

        # Create a simlink of each file in the CramSim/Tests directory
        for f in os.listdir(self.CramSimElementTestsDir):
            os_symlink_file(self.CramSimElementTestsDir, self.testCramSimTestsDir, f)

        # wget a test file tar.gz
        testfile = "sst-CramSim_trace_verimem_trace_files.tar.gz"
        fileurl = "https://github.com/sstsimulator/sst-downloads/releases/download/TestFiles/{0}".format(testfile)
        self.assertTrue(os_wget(fileurl, self.testCramSimTestsDir), "Failed to download {0}".format(testfile))

        # Extract the test file
        filename = "{0}/{1}".format(self.testCramSimTestsDir, testfile)
        os_extract_tar(filename, self.testCramSimTestsDir)

