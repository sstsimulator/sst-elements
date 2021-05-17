# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

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
            pass
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_merlin_Component(SSTTestCase):

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

    def test_merlin_dragon_128(self):
        self.merlin_test_template("dragon_128_test")

    def test_merlin_dragon_72(self):
        self.merlin_test_template("dragon_72_test")

    def test_merlin_fattree_128(self):
        self.merlin_test_template("fattree_128_test")

    def test_merlin_fattree_256(self):
        self.merlin_test_template("fattree_256_test")

    def test_merlin_torus_128(self):
        self.merlin_test_template("torus_128_test")

    def test_merlin_torus_5_trafficgen(self):
        self.merlin_test_template("torus_5_trafficgen")

    def test_merlin_torus_64(self):
         self.merlin_test_template("torus_64_test")

    def test_merlin_hyperx_128(self):
         self.merlin_test_template("hyperx_128_test")

    def test_merlin_dragon_128_platform(self):
        self.merlin_test_template("dragon_128_platform_test", True)

    def test_merlin_dragon_128_platform_cm(self):
        self.merlin_test_template("dragon_128_platform_test_cm", True)

    def test_merlin_dragon_128_fl(self):
        self.merlin_test_template("dragon_128_test_fl")


#####

    def merlin_test_template(self, testcase, cwd=False):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_merlin_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, testcase)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        if cwd:
            self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles, set_cwd=test_path)
        else:
            self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("merlin test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        cmp_result = testing_compare_sorted_diff(testcase, outfile, reffile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testcase)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(outfile, reffile))
