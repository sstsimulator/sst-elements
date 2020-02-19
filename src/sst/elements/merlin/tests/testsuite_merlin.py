# -*- coding: utf-8 -*-

import os
import filecmp

from sst_unittest_support import *

################################################################################

def setUpModule():
    test_engine_setup_module()
    # Put Module based setup code here. it is called before any testcases are run

def tearDownModule():
    # Put Module based teardown code here. it is called after all testcases are run
    test_engine_teardown_module()

################################################################################

class testcase_merlin_Component(SSTUnitTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

    def test_merlin_dragon_128(self):
        self.merlin_test_template("dragon_128_test", 500)

    def test_merlin_dragon_72(self):
        self.merlin_test_template("dragon_72_test", 500)

    def test_merlin_fattree_128(self):
        self.merlin_test_template("fattree_128_test", 500)

    def test_merlin_fattree_256(self):
        self.merlin_test_template("fattree_256_test", 500)

    def test_merlin_torus_128(self):
        self.merlin_test_template("torus_128_test", 500)

    def test_merlin_torus_5_trafficgen(self):
        self.merlin_test_template("torus_5_trafficgen", 500)

    def test_merlin_torus_64(self):
         self.merlin_test_template("torus_64_test", 500)

#####

    def merlin_test_template(self, testcase, tolerance):
        # Set the various file paths
        testDataFileName="test_merlin_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(self.get_testsuite_dir(), testcase)
        reffile = "{0}/refFiles/test_merlin_{1}.out".format(self.get_testsuite_dir(), testcase)
        outfile = "{0}/{1}.out".format(self.get_test_output_run_dir(), testDataFileName)

        self.run_sst(sdlfile, outfile)

        # Perform the test
        cmp_result = self.compare_sorted(outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

#####

    def compare_sorted(self, outfile, reffile):
       sorted_outfile = "{0}/test_merlin_sorted_outfile".format(self.get_test_output_tmp_dir())
       sorted_reffile = "{0}/test_merlin_sorted_reffile".format(self.get_test_output_tmp_dir())

       os.system("sort -o {0} {1}".format(sorted_outfile, outfile))
       os.system("sort -o {0} {1}".format(sorted_reffile, reffile))

       return filecmp.cmp(sorted_outfile, sorted_reffile)








