# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os

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
            class_inst._setupQOSTestFiles()
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_QOS(SSTTestCase):

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

    def test_qos_fattree(self):
        self.qos_test_template("fattree")

    def test_qos_dragonfly(self):
        self.qos_test_template("dragonfly", ignore_err_file=True)

    def test_qos_hyperx(self):
        self.qos_test_template("hyperx")

#####

    def qos_test_template(self, testcase, ignore_err_file=False):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.qostest_Folder = "{0}/qostest_Folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Set the various file paths
        testDataFileName="test_qos-{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        qosloadfile = "qos.load"

        qostest = "{0}/qos-{1}.sh".format(test_path, testcase)
        sdlfile = "{0}/../test/emberLoad.py".format(test_path)

        # Build the otherargs variable from the qostest file
        otherargs = ''
        with open(qostest) as fp:
            for line in fp:
                if line.find('--') == 0:
                    newline = line.replace("\\", "").replace("\n", "").replace("$LOADFILE", qosloadfile)
                    otherargs += newline
        otherargs += '\"'

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, set_cwd=self.qostest_Folder, mpi_out_files=mpioutfiles)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        if ignore_err_file is False:
            if os_test_file(errfile, "-s"):
                log_testing_note("QOS test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        ## These tests are following the older SQE bamboo based testSuite_qos.sh
        ## qos has an issue on the golden file between PY2 builds and Py3 builds
        ## due to differences in the random seed generation methods.


        # Look for a direct match
        cmp_result = testing_compare_diff(testDataFileName, outfile, reffile)
        if cmp_result:
            # Is it a direct match
            log_debug("Direct Match\n")
            self.assertTrue(cmp_result, "Diffed compared Output file {0} does not match Reference File {1}".format(outfile, reffile))
            return

        # Look for a sorted compare match
        cmp_result = testing_compare_sorted_diff(testcase, outfile, reffile)
        if cmp_result:
            # Is it a sorted match
            log_debug("Sorted Match\n")
            self.assertTrue(cmp_result, "Sorted and Diffed compared Output file {0} does not match Reference File {1}".format(outfile, reffile))
            return

        # Look for comparison of word/line counts
        ##  Reference the older SQE bamboo based testSuite_qos.sh for more info
        wc_out_data = os_wc(outfile, [0, 1])
        log_debug("{0} : wc_out_data ={1}".format(outfile, wc_out_data))
        wc_ref_data = os_wc(reffile, [0, 1])
        log_debug("{0} : wc_ref_data ={1}".format(reffile, wc_ref_data))

        wc_line_word_count_diff = wc_out_data == wc_ref_data
        if wc_line_word_count_diff:
            log_debug("Line Word Count Match\n")
        self.assertTrue(wc_line_word_count_diff, "Line & Word count between file {0} does not match Reference File {1}".format(outfile, reffile))

###############################################

    def _setupQOSTestFiles(self):
        log_debug("_setupQOSTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.qostest_Folder = "{0}/qostest_Folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Create a clean version of the qostest_Folder Directory
        if os.path.isdir(self.qostest_Folder):
            shutil.rmtree(self.qostest_Folder, True)
        os.makedirs(self.qostest_Folder)

        # Create a simlink of each file in the ember/test directory
        for f in os.listdir(self.emberelement_testdir):
            filename, ext = os.path.splitext(f)
            if ext == ".py":
                os_symlink_file(self.emberelement_testdir, self.qostest_Folder, f)

        os_symlink_file(test_path, self.qostest_Folder, "qos.load")
