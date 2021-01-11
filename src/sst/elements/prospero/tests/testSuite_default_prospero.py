# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()
pin_exec_path = ""

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        try:
            # Put your single instance Init Code Here
            class_inst._setup_prospero_test_files()
        except:
            pass
        module_init = 1
    module_sema.release()

###

def is_PIN_loaded():
    # Look to see if PIN is available
    pindir_found = False
    pin_path = os.environ.get('INTEL_PIN_DIRECTORY')
    if pin_path is not None:
        pindir_found = os.path.isdir(pin_path)
    log_debug("memHSieve Test - Intel_PIN_Path = {0}; Valid Dir = {1}".format(pin_path, pindir_found))
    return pindir_found

def is_PIN_Compiled():
    global pin_exec_path
    pin_crt = sst_elements_config_include_file_get_value_int("HAVE_PINCRT", 0, True)
    pin_exec = sst_elements_config_include_file_get_value_str("PINTOOL_EXECUTABLE", "", True)
    log_debug("memHSieve Test - Detected PIN_CRT = {0}".format(pin_crt))
    log_debug("memHSieve Test - Detected PIN_EXEC = {0}".format(pin_exec))
    pin_exec_path = pin_exec
    return pin_exec != ""

def is_Pin2_used():
    global pin_exec_path
    if is_PIN_Compiled():
        if "/pin.sh" in pin_exec_path:
            return True
        else:
            return False
    else:
        return False

def is_Pin3_used():
    global pin_exec_path
    if is_PIN_Compiled():
        if is_Pin2_used():
            return False
        else:
            # Make sure pin is at the end of the string
            pinstr = "/pin"
            idx = pin_exec_path.rfind(pinstr)
            if idx == -1:
                return False
            else:
                return (idx+len(pinstr)) == len(pin_exec_path)
    else:
        return False

################################################################################
################################################################################
################################################################################

class testcase_prospero(SSTTestCase):

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
    pin_loaded = is_PIN_loaded()

#    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
#    def test_Ariel_runstream(self):
#        self.ariel_Template("runstream", use_openmp_bin=False, use_memh=False)
#
#    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
#    def test_Ariel_testSt(self):
#        self.ariel_Template("runstreamSt", use_openmp_bin=False, use_memh=False)
#
#    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
#    def test_Ariel_testNB(self):
#        self.ariel_Template("runstreamNB", use_openmp_bin=False, use_memh=False)
#
#    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
#    def test_Ariel_memH_test(self):
#        self.ariel_Template("memHstream", use_openmp_bin=False, use_memh=True)
#
#    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
#    @unittest.skipIf(host_os_is_osx(), "Ariel: Open MP is not supported on OSX.")
#    def test_Ariel_test_ivb(self):
#        self.ariel_Template("ariel_ivb", use_openmp_bin=True, use_memh=False)
#
#    @unittest.skipIf(not pin_loaded, "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
#    @unittest.skipIf(host_os_is_osx(), "Ariel: Open MP is not supported on OSX.")
#    @unittest.skipIf(testing_check_get_num_ranks() > 1, "Ariel: test_Ariel_test_snb skipped if ranks > 1 - Sandy Bridge test is incompatible with Multi-Rank.")
#    def test_Ariel_test_snb(self):
#        self.ariel_Template("ariel_snb", use_openmp_bin=True, use_memh=False)
#


#    def test_prospero_compressed_using_TAR_traces(self):
#        # We can only test the compressed trace file on PIN2 or PIN3 With Downloaded traces
#        if [[ ${SST_USING_PIN3:+isSet} == isSet ]] && [[ ${SST_BUILD_PROSPERO_TRACE_FILE:+isSet} == isSet ]] ; then
#            skip_this_test
#            return
#        fi
#        self.prospero_template(compressed none)
#
#
#    def test_prospero_compressed_withdramsim__using_TAR_traces(self):
#        # We can only test the compressed trace file on PIN2 or PIN3 With Downloaded traces
#        if [[ ${SST_USING_PIN3:+isSet} == isSet ]] && [[ ${SST_BUILD_PROSPERO_TRACE_FILE:+isSet} == isSet ]] ; then
#            skip_this_test
#            return
#        fi
#        self.prospero_template( compressed DramSim)


    def test_prospero_text__using_TAR_traces(self):
        self.prospero_template( text none)


    def test_prospero_text_withdramsim__using_TAR_traces(self):
        self.prospero_template( text DramSim)

    def test_prospero_binary__using_TAR_traces(self):
        self.prospero_template( binary none)


    def test_prospero_binary_withdramsim__using_TAR_traces(self):
        self.prospero_template(binary DramSim)



#####

    def prospero_Template(self, testcase, use_openmp_bin=False, use_memh=False):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the paths to the various directories
        ArielElementDir = os.path.abspath("{0}/../".format(test_path))
        ArielElementFrontendDir = "{0}/frontend/simple".format(ArielElementDir)
        ArielElementStreamDir = "{0}/frontend/simple/examples/stream".format(ArielElementDir)
        ArielElementompmybarrierDir = "{0}/testopenMP/ompmybarrier".format(test_path)
        Ariel_test_stream_app = "{0}/stream".format(ArielElementStreamDir)
        Ariel_ompmybarrier_app = "{0}/ompmybarrier".format(ArielElementompmybarrierDir)
        sst_elements_parent_dir = os.path.abspath("{0}/../../../../../".format(ArielElementDir))
        memHElementsTestsDir = os.path.abspath("{0}/../memHierarchy/tests".format(ArielElementDir))

        # Set the Path to the stream applications
        os.environ["ARIEL_TEST_STREAM_APP"] = Ariel_test_stream_app
        os.environ["OMP_EXE"] = Ariel_ompmybarrier_app

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
        log_debug("memHElementsTestsDir = {0}".format(memHElementsTestsDir))
        log_debug("Env:ARIEL_TEST_STREAM_APP = {0}".format(Ariel_test_stream_app))
        log_debug("Env:OMP_EXE = {0}".format(Ariel_ompmybarrier_app))

        if use_memh == True:
          # Create a simlink of the memH ini files files
            filename = "DDR3_micron_32M_8B_x4_sg125.ini"
            filepath = "{0}/{1}".format(ArielElementFrontendDir, filename)
            if os.path.islink(filepath) == False:
                os_symlink_file(memHElementsTestsDir, ArielElementFrontendDir, filename)

            filename = "system.ini"
            filepath = "{0}/{1}".format(ArielElementFrontendDir, filename)
            if os.path.islink(filepath) == False:
                os_symlink_file(memHElementsTestsDir, ArielElementFrontendDir, filename)

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=ArielElementStreamDir, mpi_out_files=mpioutfiles)

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
        num_ref_lines = int(os_wc(reffile, [0]))
        log_debug("{0} : num_ref_lines = {1}".format(reffile, num_ref_lines))

        line_count_diff = abs(num_ref_lines - num_out_lines)
        log_debug("Line Count diff = {0}".format(line_count_diff))

        if line_count_diff > 15:
            self.assertFalse(line_count_diff > 15, "Line count between output file {0} does not match Reference File {1}; They contain {2} different lines".format(outfile, reffile, line_count_diff))

#######################

    def _setup_prospero_test_files(self):
        pass
#        # NOTE: This routine is called a single time at module startup, so it
#        #       may have some redunant
#        log_debug("_setup_ariel_test_files() Running")
#        test_path = self.get_testsuite_dir()
#        outdir = self.get_test_output_run_dir()
#
#        # Set the paths to the various directories
#        self.ArielElementDir = os.path.abspath("{0}/../".format(test_path))
#        self.ArielElementStreamDir = "{0}/frontend/simple/examples/stream".format(self.ArielElementDir)
#        self.ArielElementompmybarrierDir = "{0}/testopenMP/ompmybarrier".format(test_path)
#
#        # Now build the Ariel stream example
#        cmd = "make"
#        rtn = OSCommand(cmd, set_cwd=self.ArielElementStreamDir).run()
#        log_debug("Ariel frontend/simple/examples/stream Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
#        self.assertTrue(rtn.result() == 0, "stream.c failed to compile")
#
#        # Now build the ompmybarrier binary
#        cmd = "make"
#        rtn = OSCommand(cmd, set_cwd=self.ArielElementompmybarrierDir).run()
#        log_debug("Ariel ompmybarrier Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
#        self.assertTrue(rtn.result() == 0, "ompmybarrier.c failed to compile")


