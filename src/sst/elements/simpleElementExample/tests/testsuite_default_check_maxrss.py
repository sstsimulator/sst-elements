# -*- coding: utf-8 -*-
import os

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_check_maxrss(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_check_maxrss(self):
        self.check_maxrss_template("check_maxrss")

#####

    def check_maxrss_template(self, testcase, striptotail=0):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, "test_simpleRNGComponent_mersenne")
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        tmpfile = "{0}/{1}.tmp".format(tmpdir, testDataFileName)
        cmpfile = "{0}/{1}.cmp".format(tmpdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        self.run_sst(sdlfile, outfile, errfile, other_args='--verbose', mpi_out_files=mpioutfiles)

        # Extract the string we are looking for
        wantedline = ""
        grepstr = 'Max Resident Set'
        with open(outfile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    wantedline = line

        # Get the reference size info
        with open(reffile, 'r') as f:
            for line in f.readlines():
                refsize = float(line)


        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        self.assertFalse(os_test_file(errfile, "-s"), "check_maxrss test {0} has Non-empty Error File {1}".format(testDataFileName, errfile))

        self.assertFalse(wantedline == "", "check_maxrss test cannot find line {0}".format(grepstr))

        # Now convert the string to a list of words and extract the size and units
        linewords = wantedline.split()
        size = float(linewords[4])
        units = linewords[5]
        log_debug("Check_maxrss test: wantedline = {0}".format(wantedline))
        log_debug("Check_maxrss test: line to words = {0}".format(linewords))
        log_debug("Check_maxrss test: size = {0}; Units = {1}".format(size, units))
        log_debug("Check_maxrss test: refsize = {0}".format(refsize))

        # Convert data from MB to KB
        if units == "MB":
            size = size * 1000

        if size >  refsize:
            ratio=size/refsize
            log_debug("Check_maxrss test: size / refsize = ratio = {0}".format(ratio))
        else:
            ratio=refsize/size
            log_debug("Check_maxrss test: refsize / size = ratio = {0}".format(ratio))

        self.assertFalse(ratio > 500, "check_maxrss ratio of {0} / {1} > 500".format(wantedline, refsize))


