# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os
import inspect

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
            class_inst._setup_ariel_test_files()
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################
################################################################################
################################################################################


class testcase_Ariel(SSTTestCase):

    def initializeClass(self, testName):
        super(type(self), self).initializeClass(testName)
        # Put test based setup code here. it is called before testing starts
        # NOTE: This method is called once for every test

    def setUp(self):
        super(type(self), self).setUp()
        initializeTestModule_SingleInstance(self)
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    def file_contains(self, filename, strings):
        with open(filename, 'r') as file:
            lines = file.readlines()
            for s in strings:
                self.assertTrue(s in lines, "Output {0} does not contain expected line {1}".format(filename, s))

    pin_loaded = testing_is_PIN_loaded()

    pin_error_msg = "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist."

    # This is not an exhausitve list of tests, but it covers most of the options.

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_01(self):
        self.ariel_Template("redirect_out")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_02(self):
        self.ariel_Template("redirect_err")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_03(self):
        self.ariel_Template("redirect_out redirect_err")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_04(self):
        self.ariel_Template("redirect_in")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_05(self):
        self.ariel_Template("redirect_in redirect_out")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_06(self):
        self.ariel_Template("redirect_in redirect_err")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_07(self):
        self.ariel_Template("append_redirect_out")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_08(self):
        self.ariel_Template("append_redirect_err")

    #@unittest.skipIf(not pin_loaded, pin_error_msg)
    #@unittest.skipIf(host_os_is_osx(), "Ariel: Open MP is not supported on OSX.")
    #@unittest.skipIf(testing_check_get_num_ranks() > 1, "Ariel: test_Ariel_test_snb skipped if ranks > 1 - Sandy Bridge test is incompatible with Multi-Rank.")
    #def test_Ariel_test_snb(self):
    #    self.ariel_Template("ariel_snb")
#####

    def ariel_Template(self, args, testtimeout=30):
        # Set the paths to the various directories
        testcase = inspect.stack()[1][3] # name the test after the calling function

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set paths
        ArielElementDir = os.path.abspath("{0}/../".format(test_path))
        ArielElementAPIDir = "{0}/api".format(ArielElementDir)
        ArielElementTestIODir = "{0}/tests/testIO".format(ArielElementDir)
        ArielElementTestIOApp = "{0}/testio".format(ArielElementTestIODir)
        RefDir = "{0}/ref".format(ArielElementTestIODir)

        # Set the Path to the stream applications
        os.environ["ARIEL_EXE"] = ArielElementTestIOApp

        libpath = os.environ.get("LD_LIBRARY_PATH")
        if libpath:
            os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir + ":" + libpath
        else:
            os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir

        # Set the various file paths
        testDataFileName=("test_Ariel_{0}".format(testcase))

        sdlfile = "{0}/runtestio.py".format(ArielElementTestIODir)
        reffile = "{0}/tests/refFiles/{1}.out".format(ArielElementTestIODir, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        other_args = '--model-options="' + args +'"'

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=ArielElementTestIODir,
                     mpi_out_files=mpioutfiles, timeout_sec=testtimeout, other_args=other_args)

        testing_remove_component_warning_from_file(outfile)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Look for the word "FATAL" in the output file
        cmd = 'grep "FATAL" {0} '.format(outfile)
        grep_result = os.system(cmd) != 0
        self.assertTrue(grep_result, "Output file {0} contains the word 'FATAL'...".format(outfile))

        default_testio_stdout  = ["STDOUT: This is the default output of the testio program.\n"]
        default_testio_stderr  = ["STDERR: This is the default output of the testio program.\n"]
        redirect_testio_stdout = ["STDOUT: This file is used to test Ariel IO redirection functionality.\n",
                                  "STDOUT: This is the second line in the file.\n"]
        redirect_testio_stderr = ["STDERR: This file is used to test Ariel IO redirection functionality.\n",
                                  "STDERR: This is the second line in the file.\n"]

        gold_stdout = ""
        gold_stderr = ""

        if "redirect_out" in args:
            if "redirect_in" in args:
                gold_stdout = "{}/ref/redirect-in_stdout.txt".format(ArielElementTestIODir)
            else:
                gold_stdout = "{}/ref/default-in_stdout.txt".format(ArielElementTestIODir)

        if "redirect_err" in args:
            if "redirect_in" in args:
                gold_stderr = "{}/ref/redirect-in_stderr.txt".format(ArielElementTestIODir)
            else:
                gold_stderr = "{}/ref/default-in_stderr.txt".format(ArielElementTestIODir)

        if "append_redirect_out" in args:
            gold_stdout = "{}/ref/default-in_appendstdout.txt".format(ArielElementTestIODir)

        if "append_redirect_err" in args:
            gold_stderr = "{}/ref/default-in_appendstderr.txt".format(ArielElementTestIODir)


        # If the the sdl output is supposed to contain the program stdout, make sure it is there
        if "redirect_out" not in args and "append_redirect_out" not in args:
            if "redirect_in" in args:
                self.file_contains(outfile, redirect_testio_stdout)
            else:
                self.file_contains(outfile, default_testio_stdout)


        # If the the sdl output is supposed to contain the program stderr, make sure it is there
        if "redirect_err" not in args and "append_redirect_err" not in args:
            if "redirect_in" in args:
                self.file_contains(errfile, redirect_testio_stderr)
            else:
                self.file_contains(errfile, default_testio_stderr)

        if "redirect_out" in args or "append_redirect_out" in args:
            with open(gold_stdout, 'r') as gold:
                self.file_contains("{0}/app_stdout.txt".format(ArielElementTestIODir), gold.readlines())
        if "redirect_err" in args or "append_redirect_err" in args:
            with open(gold_stderr, 'r') as gold:
                self.file_contains("{0}/app_stderr.txt".format(ArielElementTestIODir), gold.readlines())

#######################

    def _setup_ariel_test_files(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setup_ariel_test_files() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()

        # Set the paths to the various directories
        self.ArielElementDir = os.path.abspath("{0}/../".format(test_path))
        self.ArielElementTestIODir = "{0}/tests/testIO".format(self.ArielElementDir)

        # Build the Ariel API library
        ArielApiDir = "{0}/api".format(self.ArielElementDir)
        cmd = "make"
        rtn0 = OSCommand(cmd, set_cwd=ArielApiDir).run()
        log_debug("Ariel api/libarielapi.so Make result = {0}; output =\n{1}".format(rtn0.result(), rtn0.output()))
        os.environ["ARIELAPI"] =  ArielApiDir

        # Build the testio binary
        cmd = "make testio"
        rtn1 = OSCommand(cmd, set_cwd=self.ArielElementTestIODir).run()
        log_debug("Ariel ariel/tests/testIO make result = {1}; output =\n{2}".format(ArielElementTestIODir, rtn1.result(), rtn1.output()))

        # Check that everything compiled OK
        self.assertTrue(rtn0.result() == 0, "libarielapi failed to compile")
        self.assertTrue(rtn1.result() == 0, "testio binary failed to compile")

