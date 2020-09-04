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
        # Put your single instance Init Code Here
        class_inst._setupQOSTestFiles()
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

#    def test_qos_dragonfly(self):
#        self.qos_test_template("dragonfly")

#    def test_qos_hyperx(self):
#        self.qos_test_template("hyperx")

#####

    def qos_test_template(self, testcase):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.qostest_Folder = "{0}/qostest_Folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

#        os.environ["PYTHONPATH"] = self.emberelement_testdir

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
#        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, set_cwd=test_path, mpi_out_files=mpioutfiles)

        cmp_result = compare_diff(outfile, reffile)
        self.assertTrue(cmp_result, "Diffed compared Output file {0} does not match Reference File {1}".format(outfile, reffile))

        self.assertFalse(os_test_file(errfile, "-s"), "QOS test {0} has Non-empty Error File {1}".format(testDataFileName, errfile))


###############################################

    def _setupQOSTestFiles(self):
        log_debug("_setupQOSTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = get_test_output_run_dir()
        tmpdir = get_test_output_tmp_dir()

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
                os_file_symlink(self.emberelement_testdir, self.qostest_Folder, f)

        os_file_symlink(test_path, self.qostest_Folder, "qos.load")
