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
        class_inst._setup_sieve_test_files()
        module_init = 1

    module_sema.release()

################################################################################
################################################################################
################################################################################

class testcase_memHierarchy_memHSieve(SSTTestCase):

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        initializeTestModule_SingleInstance(self)
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    # Look to see if PIN is available
    pindir_found = False
    pin_path = os.environ.get('INTEL_PIN_DIRECTORY')
    if pin_path is not None:
        pindir_found = os.path.isdir(pin_path)
    log_debug("memHSieve Test - Intel_PIN_Path = {0}; Valid Dir = {1}".format(pin_path, pindir_found))

    @unittest.skipIf(not pindir_found, "memHSieve: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_memHSieve(self):
        self.memHSieve_Template("memHSieve")

#####

    def memHSieve_Template(self, testcase):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        MemHElementDir = os.path.abspath("{0}/../".format(test_path))
        MemHElementSieveTestsDir = "{0}/Sieve/tests".format(self.MemHElementDir)
        testMemHSieveDir = "{0}/testmemhsieve".format(tmpdir)

#    testDataFileBase="test_memHSieve"
#    referenceFile="${SST_REFERENCE_ELEMENTS}/memHierarchy/Sieve/tests/refFiles/${testDataFileBase}.out"
#    csvFile="${SST_ROOT}/sst-elements/src/sst/elements/memHierarchy/Sieve/tests/StatisticOutput.csv"
#    csvFileBase="${SST_ROOT}/sst-elements/src/sst/elements/memHierarchy/Sieve/tests/StatisticOutput"
#    outFile="${SST_TEST_OUTPUTS}/${testDataFileBase}.out"
#    sutArgs="${SST_ROOT}/sst-elements/src/sst/elements/memHierarchy/Sieve/tests/sieve-test.py"

        # Set the various file paths
        testDataFileName=("test_{0}".format(testcase))
        sdlfile = "{0}/sieve-test.py".format(MemHElementSieveTestsDir, testDataFileName)
        reffile = "{0}/refFiles/{1}.out".format(MemHElementSieveTestsDir, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=testMemHSieveDir, mpi_out_files=mpioutfiles)
#        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

#        testing_remove_component_warning_from_file(outfile)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        # This test uses DRAMSim2 which dumps data to the error output, we cannot
        # test for an empty errfile.
        #self.assertFalse(os_test_file(errfile, "-s"), "hybridsim test {0} has Non-empty Error File {1}".format(testDataFileName, errfile))

#        cmp_result = testing_compare_sorted_diff(testcase, outfile, reffile)
#        self.assertTrue(cmp_result, "Diffed compared Output file {0} does not match Reference File {1}".format(outfile, reffile))

#######################

    def _setup_sieve_test_files(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setup_sieve_test_files() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.MemHElementDir = os.path.abspath("{0}/../".format(test_path))
        self.MemHElementSieveTestsDir = "{0}/Sieve/tests".format(self.MemHElementDir)
        self.testMemHSieveDir = "{0}/testmemhsieve".format(tmpdir)

        # Create a clean version of the testCramSim Directory
        if os.path.isdir(self.testMemHSieveDir):
            shutil.rmtree(self.testMemHSieveDir, True)
        os.makedirs(self.testMemHSieveDir)

        # Copy the Makefile to the test directory
        shutil.copy("{0}/Makefile".format(self.MemHElementSieveTestsDir), self.testMemHSieveDir)

        # Create a simlink of the ompsievetest.c file
        os_symlink_file(self.MemHElementSieveTestsDir, self.testMemHSieveDir, "ompsievetest.c")

        # Now run the make on it
        cmd = "make".format()
        rtn = OSCommand(cmd, set_cwd=self.testMemHSieveDir).run()
        log_debug("Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "ompsievetest.c failed to compile")








