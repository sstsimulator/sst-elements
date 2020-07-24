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
        # Put your single instance Init Code Here
        class_inst._setupCramSimTestFiles()
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
        self.saved_dir = os.getcwd()
        os.chdir(self.testCramSimDir)

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()
        os.chdir(self.saved_dir)

#####

    def test_CramSim_1_R(self):
        self.CramSim_test_template("1_R", 500)

#    def test_CramSim_1_RW(self):
#        self.CramSim_test_template("1_RW", 500)

#    def test_CramSim_1_W(self):
#        self.CramSim_test_template("1_W", 500)
#
#    def test_CramSim_2_R(self):
#        self.CramSim_test_template("2_R", 500)
#
#    def test_CramSim_2_W(self):
#        self.CramSim_test_template("2_W", 500)
#
#    def test_CramSim_4_R(self):
#        self.CramSim_test_template("4_R", 500)
#
#    def test_CramSim_4_W(self):
#        self.CramSim_test_template("4_W", 500)
#
#    def test_CramSim_5_R(self):
#        self.CramSim_test_template("5_R", 500)
#
#    def test_CramSim_5_W(self):
#        self.CramSim_test_template("5_W", 500)
#
#    def test_CramSim_6_R(self):
#        self.CramSim_test_template("6_R", 500)
#
#    def test_CramSim_6_W(self):
#        self.CramSim_test_template("6_W", 500)

#####

    def CramSim_test_template(self, testcase, tolerance):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_CramSim_{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

#        testpyfilepath = "{0}/test_txntrace.py".format(self.testCramSimTestsDir)
#        tracefile      = "{0}/sst-CramSim-trace_verimem_{1}.trc".format(self.testCramSimTestsDir, testcase)
#        configfile     = "{0}/ddr4_verimem.cfg".format(self.testCramSimDir)
        testpyfilepath = "tests/test_txntrace.py"
        tracefile      = "tests/sst-CramSim-trace_verimem_{0}.trc".format(testcase)
        configfile     = "ddr4_verimem.cfg"
        if os.path.isfile(testpyfilepath):
            sdlfile = testpyfilepath
            otherargs = '--model-options=\"--configfile={0} traceFile={1}\"'.format(configfile, tracefile)
        else:
            sdlfile = "{0}/test_txntrace4.py".format(self.testCramSimTestsDir)
            otherargs = '--model-options=\"--configfile={0} --traceFile={1}\"'.format(configfile, tracefile)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, mpi_out_files=mpioutfiles)

        # Perform the test
        cmp_result = compare_diff(outfile, reffile)
        self.assertTrue(cmp_result, "Output file {0} does not match Reference File {1}".format(outfile, reffile))

#####

    def _setupCramSimTestFiles(self):
        log_debug("_setupCramSimTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = get_test_output_run_dir()
        tmpdir = get_test_output_tmp_dir()
        self.CramSimElementDir = os.path.abspath("{0}/../".format(test_path))
        self.CramSimElementTestsDir = "{0}/tests".format(self.CramSimElementDir)
        self.testCramSimDir = "{0}/testCramSim".format(self.CramSimElementTestsDir)
        self.testCramSimTestsDir = "{0}/testCramSim/tests".format(self.CramSimElementTestsDir)

        # Create a clean version of the testCramSim Directory
        if os.path.isdir(self.testCramSimDir):
            shutil.rmtree(self.testCramSimDir, True)
        os.makedirs(self.testCramSimDir)
        os.makedirs(self.testCramSimTestsDir)

        # Create a simlink of the ddr4_verimem.cfg file
        os_file_symlink(self.CramSimElementDir, self.testCramSimDir, "ddr4_verimem.cfg")

        # Create a simlink of each file in the CramSim/Tests directory
        for f in os.listdir(self.CramSimElementTestsDir):
            os_file_symlink(self.CramSimElementTestsDir, self.testCramSimTestsDir, f)

        # wget a test file tar.gz
        testfile = "sst-CramSim_trace_verimem_trace_files.tar.gz"
        fileurl = "https://github.com/sstsimulator/sst-downloads/releases/download/TestFiles/{0}".format(testfile)
        self.assertTrue(os_wget(fileurl, self.testCramSimTestsDir), "Failed to download {0}".format(testfile))

        # Extract the test file
        filename = "{0}/{1}".format(self.testCramSimTestsDir, testfile)
        os_extract_tar(filename, self.testCramSimTestsDir)

