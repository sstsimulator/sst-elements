# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

#import os
#import shutil

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        try:
            # Put your single instance Init Code Here
            class_inst._setupSiriusZodiacTraceTestFiles()
            module_init = 1
        except:
            pass
        module_init = 1

    module_sema.release()

################################################################################

class testcase_SiriusZodiacTrace(SSTTestCase):

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        initializeTestModule_SingleInstance(self)

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    @unittest.skipIf(testing_check_get_num_ranks() > 1, "SiriusZodiacTrace: test_Sirius_Zodiac_27 skipped if Ranks > 1")
    def test_Sirius_Zodiac_27(self):
        self.SiriusZodiacTrace_test_template("27")

    def test_Sirius_Zodiac_64(self):
        self.SiriusZodiacTrace_test_template("8x8")

    def test_Sirius_Zodiac_16(self):
        self.SiriusZodiacTrace_test_template("4x4")

    def test_Sirius_Zodiac_128(self):
        self.SiriusZodiacTrace_test_template("8x8x2")

#####

    def SiriusZodiacTrace_test_template(self, testcase, testtimeout = 60):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.SiriusZodiacTraceElementDir = os.path.abspath("{0}/../".format(test_path))
        self.SiriusZodiacTraceElementTestsDir = "{0}/sirius/tests".format(self.SiriusZodiacTraceElementDir)
        self.testSiriusZodiacTraceDir = "{0}/testSiriusZodiacTrace".format(tmpdir)
        self.testSiriusZodiacTraceTestsDir = "{0}/sst/elements/zodiac/test/allreduce".format(self.testSiriusZodiacTraceDir)

        # Set the various file paths
        testDataFileName="test_Sirius_allred_{0}".format(testcase)

        reffile = "{0}/sirius/tests/refFiles/{1}.out".format(self.SiriusZodiacTraceElementDir, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        tmpfile1 = "{0}/{1}_grepped.tmp".format(outdir, testDataFileName)
        tmpfile2 = "{0}/{1}_filtered.tmp".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        sdlfile = "{0}/allreduce/allreduce.py".format(test_path)
        otherargs = '--model-options \"--shape={0}\"'.format(testcase)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles,
                     other_args=otherargs, set_cwd=self.testSiriusZodiacTraceTestsDir,
                     timeout_sec=testtimeout)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        if os_test_file(errfile, "-s"):
            log_testing_note("SiriusZodiacTrace test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        cmd = 'grep -e Total.Allreduce.Count -e Total.Allreduce.Bytes {0} > {1}'.format(outfile, tmpfile1)
        os.system(cmd)
        cmd = "awk -F: '{{$1=\"\";$6=\"\"; print }}' {0} | sort -n > {1}".format(tmpfile1, tmpfile2)
        os.system(cmd)

        cmp_result = testing_compare_diff(testDataFileName, tmpfile2, reffile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testDataFileName)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Sorted/Filtered output file {0} does not match Reference File {1}".format(tmpfile2, reffile))

#####

    def _setupSiriusZodiacTraceTestFiles(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setupSiriusZodiacTraceTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.testSiriusZodiacTraceDir = "{0}/testSiriusZodiacTrace".format(tmpdir)

        # Create a clean version of the testSiriusZodiacTrace Directory
        if os.path.isdir(self.testSiriusZodiacTraceDir):
            shutil.rmtree(self.testSiriusZodiacTraceDir, True)
        os.makedirs(self.testSiriusZodiacTraceDir)

        # wget a test file tar.gz
        testfile = "sst-Sirius-Allreduce-traces.tar.gz"
        fileurl = "https://github.com/sstsimulator/sst-downloads/releases/download/TestFiles/{0}".format(testfile)
        self.assertTrue(os_wget(fileurl, self.testSiriusZodiacTraceDir), "Failed to download {0}".format(testfile))

        # Extract the test file
        filename = "{0}/{1}".format(self.testSiriusZodiacTraceDir, testfile)
        os_extract_tar(filename, self.testSiriusZodiacTraceDir)


