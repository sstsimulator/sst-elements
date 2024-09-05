# -*- coding: utf-8 -*-
import os
import subprocess

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_mask_mpi(SSTTestCase):

    def setUp(self):
        super(testcase_mask_mpi, self).setUp()
        global module_init
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(testcase_mask_mpi, self).tearDown()

#####

    def test_sendrecv(self):
        testdir = self.get_testsuite_dir()
        libdir = sstsimulator_conf_get_value_str("SST_ELEMENT_LIBRARY","SST_ELEMENT_LIBRARY_LIBDIR")
        path = os.environ.get("SST_LIB_PATH")
        if path is None or path == "":
            os.environ["SST_LIB_PATH"] = libdir
        else:
            os.environ["SST_LIB_PATH"] = path + ":" + libdir
        self.mask_mpi_template("test_sendrecv")

    def test_reduce(self):
        testdir = self.get_testsuite_dir()
        libdir = sstsimulator_conf_get_value_str("SST_ELEMENT_LIBRARY","SST_ELEMENT_LIBRARY_LIBDIR")
        path = os.environ.get("SST_LIB_PATH")
        if path is None or path == "":
            os.environ["SST_LIB_PATH"] = libdir
        else:
            os.environ["SST_LIB_PATH"] = path + ":" + libdir
        self.mask_mpi_template("test_reduce")

    def test_alltoall(self):
        testdir = self.get_testsuite_dir()
        libdir = sstsimulator_conf_get_value_str("SST_ELEMENT_LIBRARY","SST_ELEMENT_LIBRARY_LIBDIR")
        path = os.environ.get("SST_LIB_PATH")
        if path is None or path == "":
            os.environ["SST_LIB_PATH"] = libdir
        else:
            os.environ["SST_LIB_PATH"] = path + ":" + libdir
        self.mask_mpi_template("test_alltoall")

    def test_allgather(self):
        testdir = self.get_testsuite_dir()
        libdir = sstsimulator_conf_get_value_str("SST_ELEMENT_LIBRARY","SST_ELEMENT_LIBRARY_LIBDIR")
        path = os.environ.get("SST_LIB_PATH")
        if path is None or path == "":
            os.environ["SST_LIB_PATH"] = libdir
        else:
            os.environ["SST_LIB_PATH"] = path + ":" + libdir
        self.mask_mpi_template("test_allgather")

#####

    def mask_mpi_template(self, testcase, striptotail=0):
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

        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles, set_cwd=test_path)

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
            log_testing_note("hg test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        cmp_result = testing_compare_sorted_diff(testcase, cmpfile, reffile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testcase)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Sorted Output file {0} does not match sorted Reference File {1}".format(cmpfile, reffile))
