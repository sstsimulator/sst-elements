# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os


class testcase_Ariel(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test
        self._setup_ariel_test_files()

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    pin_loaded = testing_is_PIN_loaded()

    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist.")
    def test_Ariel_runstream(self):
        self.ariel_Template("runstream")

    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist.")
    def test_Ariel_testSt(self):
        self.ariel_Template("runstreamSt")

    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist.")
    def test_Ariel_testNB(self):
        self.ariel_Template("runstreamNB")

    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist.")
    def test_Ariel_memH_test(self):
        self.ariel_Template("memHstream")

    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist.")
    @unittest.skipIf(host_os_is_osx(), "Ariel: Open MP is not supported on OSX.")
    def test_Ariel_test_ivb(self):
        self.ariel_Template("ariel_ivb")

    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist.")
    @unittest.skipIf(host_os_is_osx(), "Ariel: Open MP is not supported on OSX.")
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Ariel: test_Ariel_test_snb skipped if ranks > 1 - Sandy Bridge test is incompatible with Multi-Rank.")
    def test_Ariel_test_snb(self):
        self.ariel_Template("ariel_snb")

    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist.")
    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Ariel: test_Ariel_test_snb_mlm skipped if ranks > 1 - Sandy Bridge test is incompatible with Multi-Rank.")
    def test_Ariel_test_snb_mlm(self):
        self.ariel_Template("ariel_snb_mlm", app="stream_mlm")
#####

    def ariel_Template(self, testcase, app="", testtimeout=480):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the paths to the various directories
        ArielElementDir = os.path.abspath("{0}/../".format(test_path))
        ArielElementFrontendDir = "{0}/frontend/simple".format(ArielElementDir)
        ArielElementStreamDir = "{0}/frontend/simple/examples/stream".format(ArielElementDir)
        ArielElementompmybarrierDir = "{0}/testopenMP/ompmybarrier".format(test_path)
        ArielElementAPIDir = "{0}/api".format(ArielElementDir)
        Ariel_test_stream_app = "{0}/stream".format(ArielElementStreamDir)
        Ariel_ompmybarrier_app = "{0}/ompmybarrier".format(ArielElementompmybarrierDir)
        Ariel_test_stream_mlm_app = "{0}/stream_mlm".format(ArielElementStreamDir)
        sst_elements_parent_dir = os.path.abspath("{0}/../../../../../".format(ArielElementDir))

        # Set the Path to the stream applications
        os.environ["ARIEL_TEST_STREAM_APP"] = Ariel_test_stream_app
        if app == "":
            os.environ["OMP_EXE"] = Ariel_ompmybarrier_app
        elif app == "stream_mlm":
            os.environ["OMP_EXE"] = Ariel_test_stream_mlm_app
            libpath = os.environ.get("LD_LIBRARY_PATH")
            if libpath:
                os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir + ":" + libpath
            else:
                os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir
            # Also simlink in malloc.txt
            filepath = "{0}/malloc.txt".format(ArielElementFrontendDir)
            if os.path.islink(filepath) == False:
                os_symlink_file(ArielElementStreamDir, ArielElementFrontendDir, "malloc.txt")
        else:
            os.environ["OMP_EXE"] = app # Probably will fail unless this path is correctly set, but will give a useful error message

        # Set the various file paths
        testDataFileName=("test_Ariel_{0}".format(testcase))
        sdlfile = "{0}/{1}.py".format(ArielElementStreamDir, testcase)
        reffile = "{0}/tests/refFiles/{1}.out".format(ArielElementStreamDir, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))
        log_debug("sst_elements_parent_dir = {0}".format(sst_elements_parent_dir))
        log_debug("Env:ARIEL_TEST_STREAM_APP = {0}".format(Ariel_test_stream_app))
        log_debug("Env:OMP_EXE = {0}".format(Ariel_ompmybarrier_app))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=ArielElementStreamDir,
                     mpi_out_files=mpioutfiles, timeout_sec=testtimeout)

        testing_remove_component_warning_from_file(outfile)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Look for the word "FATAL" in the output file
        cmd = 'grep "FATAL" {0} '.format(outfile)
        grep_result = os.system(cmd) != 0
        self.assertTrue(grep_result, "Output file {0} contains the word 'FATAL'...".format(outfile))

        num_out_lines  = int(os_wc(outfile, [0]))
        log_debug("{0} : num_out_lines = {1}".format(outfile, num_out_lines))
        num_err_lines  = int(os_wc(errfile, [0]))
        log_debug("{0} : num_err_lines = {1}".format(errfile, num_err_lines))
        num_ref_lines = int(os_wc(reffile, [0]))
        log_debug("{0} : num_ref_lines = {1}".format(reffile, num_ref_lines))

        line_count_diff = abs(num_ref_lines - num_out_lines - num_err_lines)
        log_debug("Line Count diff = {0}".format(line_count_diff))

        delta = 15
        if line_count_diff > delta:
            self.assertFalse(line_count_diff > 15, f"Test stdout ({outfile}) and stderr ({errfile}) contain {num_out_lines}+{num_err_lines}={num_out_lines+num_err_lines} lines. Expected this to be within {delta} of the reference file ({reffile}), which has {num_ref_lines} lines, but the difference is {line_count_diff} lines.")

#######################

    def _setup_ariel_test_files(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setup_ariel_test_files() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()

        # Set the paths to the various directories
        self.ArielElementDir = os.path.abspath("{0}/../".format(test_path))
        self.ArielElementStreamDir = "{0}/frontend/simple/examples/stream".format(self.ArielElementDir)
        self.ArielElementompmybarrierDir = "{0}/testopenMP/ompmybarrier".format(test_path)

        # Add the API directory to LD_LIBRARY_PATH
        # It is set in the Makefile but we set it here as well
        # to support out-of-source builds
        #ArielApiDir = "{0}/api".format(self.ArielElementDir)
        ElementsBuildDir = sstsimulator_conf_get_value_str("SST_ELEMENT_LIBRARY", "SST_ELEMENT_LIBRARY_BUILDDIR")
        ArielApiDir = "{0}/src/sst/elements/ariel/api".format(ElementsBuildDir)
        current_ld_library_path = os.environ.get("LD_LIBRARY_PATH", "")

        OSCommand("make", set_cwd=ArielApiDir).run()
        OSCommand("cp {0}/libarielapi.so {1}/libarielapi.so".format(ArielApiDir,self.ArielElementStreamDir)).run()

        if current_ld_library_path != "":
            new_ld_library_path = f"{current_ld_library_path}:{ArielApiDir}"
        else:
            new_ld_library_path = ArielApiDir

        os.environ["LD_LIBRARY_PATH"] = new_ld_library_path

        # Now build the Ariel stream example
        cmd = "make"
        rtn1 = OSCommand(cmd, set_cwd=self.ArielElementStreamDir).run()
        log_debug("Ariel stream Make result = {0}; output =\n{1}".format(rtn1.result(), rtn1.output()))
        if rtn1.result() != 0:
            log_debug("Ariel {0} Make returned non-zero exit code. Error:\n{1}".format(self.ArielElementStreamDir, rtn1.error()))


        # Now build the ompmybarrier binary
        cmd = "make"
        rtn2 = OSCommand(cmd, set_cwd=self.ArielElementompmybarrierDir).run()
        log_debug("Ariel ompmybarrier Make result = {0}; output =\n{1}".format(rtn2.result(), rtn2.output()))
        if rtn2.result() != 0:
            log_debug("Ariel {0} Make returned non-zero exit code. Error:\n{1}".format(self.ArielElementompmybarrierDir, rtn2.error()))

        # Check that everything compiled OK
        #self.assertTrue(rtn0.result() == 0, "libarielapi failed to compile")
        self.assertTrue(rtn1.result() == 0, "stream apps failed to compile.\nResult was {0}.\nStandard out was:\n{1}\nStandard err was:\n{2}".format(rtn1.result(), rtn1.output(), rtn1.error()))
        self.assertTrue(rtn2.result() == 0, "ompmybarrier.c failed to compile")

