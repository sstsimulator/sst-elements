# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_merlin_Component(SSTTestCase):

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
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = get_test_output_run_dir()

        # Set the various file paths
        testDataFileName="test_merlin_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, testcase)
        reffile = "{0}/refFiles/test_merlin_{1}.out".format(test_path, testcase)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

        # Perform the test
        cmp_result = compare_sorted(testcase, outfile, reffile)
        self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(outfile, reffile))
