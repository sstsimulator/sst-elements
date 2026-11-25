# -*- coding: utf-8 -*-

import fcntl
import os
import shutil

from sst_unittest import *
from sst_unittest_support import *


class testcase_cramSim_Component(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        self._setupcramSimTestFiles()

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_cramSim_1_R(self):
        self.cramSim_test_template("1_R")

    def test_cramSim_1_RW(self):
        self.cramSim_test_template("1_RW")

    def test_cramSim_1_W(self):
        self.cramSim_test_template("1_W")

    def test_cramSim_2_R(self):
        self.cramSim_test_template("2_R")

    def test_cramSim_2_W(self):
        self.cramSim_test_template("2_W")

    def test_cramSim_4_R(self):
        self.cramSim_test_template("4_R")

    def test_cramSim_4_W(self):
        self.cramSim_test_template("4_W")

    def test_cramSim_5_R(self):
        self.cramSim_test_template("5_R")

    def test_cramSim_5_W(self):
        self.cramSim_test_template("5_W")

    def test_cramSim_6_R(self):
        self.cramSim_test_template("6_R")

    def test_cramSim_6_W(self):
        self.cramSim_test_template("6_W")

#####

    def cramSim_test_template(self, testcase):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()

        # Set the various file paths
        testDataFileName="test_cramSim_{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        testpyfilepath = "{0}/test_txntrace.py".format(self.testcramSimTestsDir)
        tracefile      = "{0}/sst-CramSim-trace_verimem_{1}.trc".format(self.testcramSimTestsDir, testcase)
        configfile     = "{0}/ddr4_verimem.cfg".format(self.testcramSimDir)

        if os.path.isfile(testpyfilepath):
            sdlfile = testpyfilepath
            otherargs = '--model-options=\"--configfile={0} --traceFile={1}\"'.format(configfile, tracefile)
        else:
            sdlfile = "{0}/test_txntrace4.py".format(self.testcramSimTestsDir)
            otherargs = '--model-options=\"--configfile={0} --traceFile={1}\"'.format(configfile, tracefile)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, mpi_out_files=mpioutfiles)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("cramSim test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        # NOTE: This is how the bamboo tests does it, and its very crude.  The
        #       testing_compare_diff will always fail, so all it looks for is the
        #       "Simulation complete" message to decide pass/fail
        #       This should be improved upon to check for real data...
        cmp_result = testing_compare_diff(testDataFileName, outfile, reffile)
        if not cmp_result:
            cmd = 'grep -q "Simulation is complete" {0} '.format(outfile)
            grep_result = os.system(cmd) == 0
            self.assertTrue(grep_result, "Output file {0} does not contain a simulation complete message".format(outfile, reffile))
        else:
            self.assertTrue(cmp_result, "Output file {0} does not match Reference File {1}".format(outfile, reffile))

#####

    def _setupcramSimTestFiles(self):
        # NOTE: This routine is called before every test, so each test gets
        #       its own copy of the test files
        log_debug("_setupcramSimTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        tarfile = self._download_cramSim_trace_files()

        self.cramSimElementDir = os.path.abspath("{0}/../".format(test_path))
        self.cramSimElementTestsDir = "{0}/tests".format(self.cramSimElementDir)
        self.testcramSimDir = "{0}/testcramSim/{1}".format(tmpdir, self.testname)
        self.testcramSimTestsDir = "{0}/tests".format(self.testcramSimDir)

        # Create a clean version of the testcramSim Directory
        if os.path.isdir(self.testcramSimDir):
            shutil.rmtree(self.testcramSimDir, True)
        os.makedirs(self.testcramSimDir)
        os.makedirs(self.testcramSimTestsDir)

        # Create a simlink of the ddr4_verimem.cfg file
        os_symlink_file(self.cramSimElementDir, self.testcramSimDir, "ddr4_verimem.cfg")

        # Create a simlink of each file in the cramSim/Tests directory
        for f in os.listdir(self.cramSimElementTestsDir):
            os_symlink_file(self.cramSimElementTestsDir, self.testcramSimTestsDir, f)

        # Extract the test file
        os_extract_tar(tarfile, self.testcramSimTestsDir)

    def _download_cramSim_trace_files(self):
        # Concurrent tests share one download, so only one test at a time
        # may check for it and download it.
        tmpdir = self.get_test_output_tmp_dir()
        testfile = "sst-cramSim_trace_verimem_trace_files.tar.gz"
        fileurl = "https://github.com/sstsimulator/sst-downloads/releases/download/TestFiles/{0}".format(testfile)
        downloaddir = "{0}/cramSim_download".format(tmpdir)
        tarfile = "{0}/{1}".format(downloaddir, testfile)

        lockfile = "{0}/cramSim_download.lock".format(tmpdir)
        with open(lockfile, "w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            if not os.path.isfile(tarfile):
                os.makedirs(downloaddir, exist_ok=True)
                if not os_wget(fileurl, downloaddir):
                    # Remove a partial file, so the next test tries again
                    if os.path.isfile(tarfile):
                        os.remove(tarfile)
                    self.skipTest("Failed to download {0}".format(testfile))
        return tarfile
