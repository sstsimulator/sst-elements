# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *


class testcase_kingsley_Component(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_kingsly_noc_mesh_32(self):
        self.kingsley_test_template("noc_mesh_32_test")

#####

    def kingsley_test_template(self, testcase):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_kingsley_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, testcase)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("kingsley test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        cmp_result = testing_compare_sorted_diff(testcase, outfile, reffile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testcase)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(outfile, reffile))
