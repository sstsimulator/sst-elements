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

class testcase_Messier_Component(SSTTestCase):

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

    def test_Messier_gupsgen(self):
        self.Messier_test_template("gupsgen")

    def test_Messier_gupsgen_2RANKS(self):
        self.Messier_test_template("gupsgen_2RANKS")

    def test_Messier_gupsgen_fastNVM(self):
        self.Messier_test_template("gupsgen_fastNVM")

    def test_Messier_stencil3dbench_messier(self):
        self.Messier_test_template("stencil3dbench_messier")

    def test_Messier_streambench_messier(self):
        self.Messier_test_template("streambench_messier")

#####

    def Messier_test_template(self, testcase, testtimeout=240):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_Messier_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, testcase)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        newreffile = "{0}/refFiles/{1}.newref".format(outdir, testDataFileName)
        newoutfile = "{0}/{1}.newout".format(outdir, testDataFileName)

        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles, timeout_sec=testtimeout)

        testing_remove_component_warning_from_file(outfile)

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("Messier test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the test
        cmp_result = testing_compare_sorted_diff(testcase, outfile, reffile)

        # Special case handling of stencil3dbench_messier
        if not cmp_result and testcase == "stencil3dbench_messier":
            ##  Follows some bailing wire to allow serialization branch to work with same reference files
            ##  Reference the older SQE bamboo based testSuite_Messier.sh for more info
            wc_out_data = os_wc(outfile, [0, 1])
            log_debug("{0} : wc_out_data ={1}".format(outfile, wc_out_data))
            wc_ref_data = os_wc(reffile, [0, 1])
            log_debug("{0} : wc_ref_data ={1}".format(reffile, wc_ref_data))
            wc_line_word_count_diff = wc_out_data == wc_ref_data
            self.assertTrue(wc_line_word_count_diff, "Line & Word count between file {0} does not match Reference File {1}".format(outfile, reffile))

        else:
            if (cmp_result == False):
                diffdata = testing_get_diff_data(testcase)
                log_failure(diffdata)
            self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(outfile, reffile))
