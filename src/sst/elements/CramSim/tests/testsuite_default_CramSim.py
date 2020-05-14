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
        class_inst.setupCramSimTestFiles()
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

    def setupCramSimTestFiles(self):
        log_debug("\nsetupCramSimTestFiles()")
        test_path = self.get_testsuite_dir()
        outdir = get_test_output_run_dir()
        tmpdir = get_test_output_tmp_dir()
        CramSimElementDir = os.path.abspath("{0}/../".format(test_path))
        CramSimElementTestsDir = "{0}/tests".format(CramSimElementDir)
        testCramSimDir = "{0}/testCramSim".format(tmpdir)
        testCramSimTestsDir = "{0}/testCramSim/tests".format(tmpdir)

        # Create a clean version of the testCramSim Directory
        if os.path.isdir(testCramSimDir):
            shtuil.rmtree(testCramSimDir, True)
        os.makedirs(testCramSimDir)
        os.makedirs(testCramSimTestsDir)

        # Create a simlink of the ddr4_verimem.cfg file
        os_file_symlink(CramSimElementDir, testCramSimDir, "ddr4_verimem.cfg")

        # Create a simlink of each file in the CramSim/Tests directory
        for f in os.listdir(CramSimElementTestsDir):
            os_file_symlink(CramSimElementTestsDir, testCramSimTestsDir, f)

        # wget a test file tar.gz
        testfile = "sst-CramSim_trace_verimem_trace_files.tar.gz"
        fileurl = "https://github.com/sstsimulator/sst-downloads/releases/download/TestFiles/{0}".format(testfile)
        self.assertTrue(os_wget(fileurl, testCramSimTestsDir), "Failed to download {0}".format(testfile))

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
        outdir = get_test_output_run_dir()
        tmpdir = get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_CramSim_{0}trc".format(testcase)

##        sdlfile = "{0}/{1}.py".format(test_path, testcase)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        newoutfile = "{0}/{1}.newout".format(outdir, testDataFileName)
        newreffile = "{0}/{1}.newref".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        return



        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

        # Perform the test
        cmp_result = compare_sorted(testcase, outfile, reffile)
        self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(outfile, reffile))



    def _CramSim_test_setup():
        log_forced("\nCRAMSIM Performing Test SETUP")
        if self._moduleInitialized == 1:
            print("AARON - CRAMSIM SETUP ALREADY PERFORMED")
        else:
            print("AARON - PERFORMING CRAMSIM SETUP")




