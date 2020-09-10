# -*- coding: utf-8 -*-

import os
import filecmp

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_simpleRNGComponent(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_RNG_Mersenne(self):
        self.RNG_test_template("mersenne")


    def test_RNG_Marsaglia(self):
        self.RNG_test_template("marsaglia")


    def test_RNG_xorshift(self):
        self.RNG_test_template("xorshift")

#####

    def RNG_test_template(self, testcase):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        sdlfile = "{0}/test_simpleRNGComponent_{1}.py".format(test_path, testcase)
        reffile = "{0}/refFiles/test_simpleRNGComponent_{1}.out".format(test_path, testcase)
        outfile = "{0}/test_simpleRNGComponent_{1}.out".format(outdir, testcase)
        tmpfile = "{0}/test_simpleRNGComponent_{1}.tmp".format(tmpdir, testcase)
        cmpfile = "{0}/test_simpleRNGComponent_{1}.cmp".format(tmpdir, testcase)

        self.run_sst(sdlfile, outfile)

        # Post processing of the output data to scrub it into a format to compare
        os.system("grep Random {0} > {1}".format(outfile, tmpfile))
        os.system("tail -5 {0} > {1}".format(tmpfile, cmpfile))

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the test
        testresult = filecmp.cmp(cmpfile, reffile)
        testerror = "Output/Compare file {0} does not match Reference File {1}".format(cmpfile, reffile)
        self.assertTrue(testresult, testerror)
