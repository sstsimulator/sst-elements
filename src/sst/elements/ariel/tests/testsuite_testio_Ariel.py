# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os
import inspect


##############################################
# EPA Frontend Functions not yet in sst-core #
##############################################
def is_EPAX_loaded() -> bool:
    # Check if arm-based epa tool is available
    epaxdir_found = False
    epax_path = os.environ.get('EPAX_ROOT')
    if epax_path is not None:
        epaxdir_found = os.path.isdir(epax_path)
    return epaxdir_found

def is_PEBIL_loaded() -> bool:
    # Check if x86-based epa tool is available
    pebildir_found = False
    pebil_path = os.environ.get('PEBIL_ROOT')
    if pebil_path is not None:
        pebildir_found = os.path.isdir(pebil_path)
    return pebildir_found

class testcase_Ariel(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test
        self._setup_ariel_test_files()

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
    epax_loaded = is_EPAX_loaded()
    pebil_loaded = is_PEBIL_loaded()
    epa_loaded = epax_loaded or pebil_loaded

    pin_error_msg = "Ariel: PIN tests require PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist."
    epa_error_msg = "Ariel: EPA tests require PEBIL or EPAX, but Env Var 'PEBIL_ROOT' or 'EPAX_ROOT' is not found or path does not exist."

    # This is not an exhausitve list of tests, but it covers most of the options.

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_01_pin(self):
        self.ariel_Template("redirect_out")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_01_epa(self):
        self.ariel_Template("redirect_out", frontend="epa")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_02_pin(self):
        self.ariel_Template("redirect_err")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_02_epa(self):
        self.ariel_Template("redirect_err", frontend="epa")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_03_pin(self):
        self.ariel_Template("redirect_out redirect_err")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_03_epa(self):
        self.ariel_Template("redirect_out redirect_err", frontend="epa")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_04_pin(self):
        self.ariel_Template("redirect_in")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_04_epa(self):
        self.ariel_Template("redirect_in", frontend="epa")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_05_pin(self):
        self.ariel_Template("redirect_in redirect_out")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_05_epa(self):
        self.ariel_Template("redirect_in redirect_out", frontend="epa")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_06_pin(self):
        self.ariel_Template("redirect_in redirect_err")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_06_epa(self):
        self.ariel_Template("redirect_in redirect_err", frontend="epa")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_07_pin(self):
        self.ariel_Template("append_redirect_out")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_07_epa(self):
        self.ariel_Template("append_redirect_out", frontend="epa")

    @unittest.skipIf(not pin_loaded, pin_error_msg)
    def test_testio_08_pin(self):
        self.ariel_Template("append_redirect_err")

    @unittest.skipIf(not epa_loaded, epa_error_msg)
    def test_testio_08_epa(self):
        self.ariel_Template("append_redirect_err", frontend="epa")

    #@unittest.skipIf(not pin_loaded, pin_error_msg)
    #@unittest.skipIf(host_os_is_osx(), "Ariel: Open MP is not supported on OSX.")
    #@unittest.skipIf(testing_check_get_num_ranks() > 1, "Ariel: test_Ariel_test_snb skipped if ranks > 1 - Sandy Bridge test is incompatible with Multi-Rank.")
    #def test_Ariel_test_snb(self):
    #    self.ariel_Template("ariel_snb")
#####

    def ariel_Template(self, args, testtimeout=30, frontend="pin"):
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

        # If epa frontend, use instrumented application
        if frontend == "epa":
            ArielElementTestIOApp = ArielElementTestIOApp + ".arielinst"

        # Set the Path to the stream applications
        os.environ["ARIEL_EXE"] = ArielElementTestIOApp
        log_debug("ARIEL_EXE = {0}".format(ArielElementTestIOApp))

        libpath = os.environ.get("LD_LIBRARY_PATH")
        if libpath:
            os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir + ":" + libpath
        else:
            os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir

        # Set the frontend (epa or pin)
        os.environ["ARIEL_TEST_FRONTEND"] = frontend

        # Set the various file paths
        testDataFileName=("test_Ariel_{0}".format(testcase))

        sdlfile = "{0}/runtestio.py".format(ArielElementTestIODir)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        other_args = '--model-options="' + args +'"'

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
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


        # If the the sdl stdout is supposed to contain the program stdout, make sure it is there
        if "redirect_out" not in args and "append_redirect_out" not in args:
            if "redirect_in" in args:
                self.file_contains(outfile, redirect_testio_stdout)
            else:
                self.file_contains(outfile, default_testio_stdout)


        # If the the sdl stderr is supposed to contain the program stderr, make sure it is there
        if "redirect_err" not in args and "append_redirect_err" not in args:
            if "redirect_in" in args:
                self.file_contains(errfile, redirect_testio_stderr)
            else:
                self.file_contains(errfile, default_testio_stderr)

        # Pick the correct program stdout and stderr files based on the test settings
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

        log_debug("gold_stdout = {0}".format(gold_stdout))
        log_debug("gold_stderr = {0}".format(gold_stderr))

        # Check that the specified out and err files contain the program output. There is also
        # some Ariel output which may change in the future so we don't diff the files. We just
        # check to see that the expected lines are there.
        if "redirect_out" in args or "append_redirect_out" in args:
            log_debug("redirect_out_file = {0}/app_stdout.txt".format(ArielElementTestIODir))
            with open(gold_stdout, 'r') as gold:
                self.file_contains("{0}/app_stdout.txt".format(ArielElementTestIODir), gold.readlines())

        if "redirect_err" in args or "append_redirect_err" in args:
            log_debug("redirect_err_file = {0}/app_stderr.txt".format(ArielElementTestIODir))
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
        log_debug("Ariel ariel/tests/testIO make result = {1}; output =\n{2}".format(self.ArielElementTestIODir, rtn1.result(), rtn1.output()))

        # Check that everything compiled OK
        self.assertTrue(rtn0.result() == 0, "libarielapi failed to compile")
        self.assertTrue(rtn1.result() == 0, "testio binary failed to compile")

        # Everything compiled OK. Check if we need to check for epa binaries
        epax_loaded = is_EPAX_loaded()
        pebil_loaded = is_PEBIL_loaded()
        epa_loaded = epax_loaded or pebil_loaded

        if not epa_loaded:
            return

        # If doing epa frontend tests, check if testio is instrumented.
        # If not, instrument it
        if (not os.path.exists(self.ArielElementTestIODir + "/testio.arielinst")):
            # Instrument with pebil
            if (pebil_loaded):
                cmd = "pebil --tool ArielIntercept --app testio --inp +"
                rtn2 = OSCommand(cmd, set_cwd=self.ArielElementTestIODir).run()
                log_debug("Ariel ariel/tests/testIO pebil result = {0}; output =\n{1}".format(rtn2.result(), rtn2.output()))
                if rtn2.result() != 0:
                    log_debug("Ariel {0} pebil returned non-zero exit code. Error:\n{1}".format(self.ArielElementTestIODir, rtn2.error()))
            # Instrument with epax
            else:
                self.assertTrue(False, "epax commands not implemented yet")
            # Check that everything instrumented OK
            self.assertTrue(rtn2.result() == 0, "testio app failed to instrument.\nResult was {0}.\nStandard out was:\n{1}\nStandard err was:\n{2}".format(rtn2.result(), rtn2.output(), rtn2.error()))
