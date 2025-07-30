# -*- coding: utf-8 -*-
from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_simpleSimulation(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    # Test simpleCarWash. The test has one component so only run it if SST is running serially
    parallel = testing_check_get_num_threads() * testing_check_get_num_ranks()
    @unittest.skipIf(parallel > 1 , "Test has only one component but SST is running in parallel")
    def test_simplesimulation_simplecarwash(self):
        self.simplesim_template("simpleCarWash")

    @unittest.skipIf(parallel > 2, "Test has two components but SST is running with more threads")
    def test_simplesimulation_distcarwash(self):
        self.simplesim_template("distCarWash")

#####

    def simplesim_template(self, testcase):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, testDataFileName)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

        testing_remove_component_warning_from_file(outfile)

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("simpleSimulation test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        cmp_result = testing_compare_sorted_diff(testcase, outfile, reffile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testcase)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(outfile, reffile))
