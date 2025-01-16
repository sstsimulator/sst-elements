# -*- coding: utf-8 -*-
import os

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_simpleComponents(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    def test_example_0(self):
        self.simple_components_template("example0")

    def test_example_1(self):
        self.simple_components_template("example1")

    # Only has one component, skip if parallelism is > 1
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "SimpleElementExample: basicClocks skipped if ranks > 1 - single component in config.")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "SimpleElementExample: basicClocks skipped if threads > 1 - single component in config.")
    def test_basic_clocks(self):
        self.simple_components_template("basicClocks")

    def test_basic_links(self):
        self.simple_components_template("basicLinks")

    # Only has one component, skip if parallelism is > 1
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "SimpleElementExample: basicParams skipped if ranks > 1 - single component in config.")
    @unittest.skipIf(testing_check_get_num_threads() > 1, "SimpleElementExample: basicParams skipped if threads > 1 - single component in config.")
    def test_basic_params(self):
        self.simple_components_template("basicParams")

    def test_basic_statistics_0(self):
        self.simple_components_template("basicStatistics0")

    def test_basic_statistics_1(self):
        self.simple_components_template("basicStatistics1")

    def test_basic_statistics_2(self):
        self.simple_components_template("basicStatistics2")
    
    def test_basic_simlifecycle(self):
        self.simple_components_template("basicSimLifeCycle")
    
    def test_basic_subcomponent(self):
        self.simple_components_template("basicSubComponent")

    def test_basic_checkpoint(self):
        self.simple_components_template("basicCheckpoint", checkpoint=True)

    #def test_simple_rng_component_marsaglia(self):
    #    self.simple_components_template("simpleRNGComponent_marsaglia", striptotail=1)

#####

    def simple_components_template(self, testcase, striptotail=0, checkpoint=False):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, testDataFileName)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        tmpfile = "{0}/{1}.tmp".format(tmpdir, testDataFileName)
        cmpfile = "{0}/{1}.cmp".format(tmpdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        if not checkpoint:
            self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

            testing_remove_component_warning_from_file(outfile)

            # Copy the outfile to the cmpfile
            os.system("cp {0} {1}".format(outfile, cmpfile))

            if striptotail == 1:
                # Post processing of the output data to scrub it into a format to compare
                os.system("grep Random {0} > {1}".format(outfile, tmpfile))
                os.system("tail -5 {0} > {1}".format(tmpfile, cmpfile))

            # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
            #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
            #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
            #       TESTS & RESULT FILES ARE STILL VALID

            # Perform the tests
            if os_test_file(errfile, "-s"):
                log_testing_note("simpleComponents test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

            cmp_result = testing_compare_sorted_diff(testcase, cmpfile, reffile)
            if (cmp_result == False):
                diffdata = testing_get_diff_data(testcase)
                log_failure(diffdata)
            self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(cmpfile, reffile))

        # Checkpoint test
        else:
            cptfreq = "15us"
            cptrestart = "0_15000000"

            # Generate checkpoint
            sdlfile_generate = "{0}/{1}.py".format(test_path,testcase)
            outfile_generate = "{0}/{1}_generate.out".format(outdir,testcase)
            options_checkpoint="--checkpoint-sim-period={0} --checkpoint-prefix={1}".format(cptfreq,testcase)
            self.run_sst(sdlfile_generate, outfile_generate, other_args=options_checkpoint)

            # Run from restart
            sdlfile_restart = "{0}/{1}/{1}_{2}/{1}_{2}.sstcpt".format(outdir,testcase,cptrestart)
            outfile_restart = "{0}/{1}_restart.out".format(outdir, testcase)
            options_restart = "--load-checkpoint"
            self.run_sst(sdlfile_restart, outfile_restart, other_args=options_restart)

            # Check that restart output is a subset of checkpoint output
            cmp_result = testing_compare_filtered_subset(outfile_restart, outfile_generate)
            self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile_restart, outfile_generate))