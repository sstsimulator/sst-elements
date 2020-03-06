# -*- coding: utf-8 -*-

import sst_unittest_support
from sst_unittest_support import *

################################################################################

def setUpModule():
    sst_unittest_support.setUpModule()
    # Put Module based setup code here. it is called before any testcases are run

def tearDownModule():
    # Put Module based teardown code here. it is called after all testcases are run
    sst_unittest_support.tearDownModule()

################################################################################

class testcase_merlin_Component(SSTTestCase):

    @classmethod
    def setUpClass(cls):
        super(cls, cls).setUpClass()
        # Put class based setup code here. it is called once before tests are run

    @classmethod
    def tearDownClass(cls):
        # Put class based teardown code here. it is called once after tests are run
        super(cls, cls).tearDownClass()

#####

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

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

        sdlfile = "{0}/{1}.py".format(get_testsuite_dir(), testcase)
        reffile = "{0}/refFiles/test_merlin_{1}.out".format(get_testsuite_dir(), testcase)
        outfile = "{0}/{1}.out".format(get_test_output_run_dir(), testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(get_test_output_run_dir(), testDataFileName)

        self.run_sst(sdlfile, outfile, mpi_out_files=mpioutfiles)

        # Perform the test
        cmp_result = compare_sorted(testcase, outfile, reffile)
        self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))
