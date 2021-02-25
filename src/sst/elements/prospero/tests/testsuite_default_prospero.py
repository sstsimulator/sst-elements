# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os
import glob

USE_PIN_TRACES = True
USE_TAR_TRACES = False
WITH_DRAMSIM = True
NO_DRAMSIM = False

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
            class_inst._setup_prospero_test_dirs()
            class_inst._create_prospero_PIN_trace_files()
            class_inst._download_prospero_TAR_trace_files()
        except:
            pass
        module_init = 1
    module_sema.release()

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
    pin_loaded = testing_is_PIN_loaded()
    pin3_used = testing_is_PIN3_used()
    libz_missing = not sst_elements_config_include_file_get_value_int("HAVE_LIBZ", default=0, disable_warning=True)

    @unittest.skipIf(libz_missing, "test_prospero_compressed_using_TAR_traces test: Requires LIBZ, but LIBZ is not found in build configuration.")
    def test_prospero_compressed_using_TAR_traces(self):
        self.prospero_test_template("compressed", NO_DRAMSIM, USE_TAR_TRACES)

    @unittest.skipIf(libz_missing, "test_prospero_compressed_withdramsim_using_TAR_traces test: Requires LIBZ, but LIBZ is not found in build configuration.")
    def test_prospero_compressed_withdramsim_using_TAR_traces(self):
        self.prospero_test_template("compressed", WITH_DRAMSIM, USE_TAR_TRACES)

    def test_prospero_text_using_TAR_traces(self):
        self.prospero_test_template("text", NO_DRAMSIM, USE_TAR_TRACES)

    def test_prospero_binary_using_TAR_traces(self):
        self.prospero_test_template("binary", NO_DRAMSIM, USE_TAR_TRACES)

    def test_prospero_text_withdramsim_using_TAR_traces(self):
        self.prospero_test_template("text", WITH_DRAMSIM, USE_TAR_TRACES)

    def test_prospero_binary_withdramsim_using_TAR_traces(self):
        self.prospero_test_template("binary", WITH_DRAMSIM, USE_TAR_TRACES)

    @unittest.skipIf(libz_missing, "test_prospero_compressed_using_PIN_traces test: Requires LIBZ, but LIBZ is not found in build configuration.")
    @unittest.skipIf(not pin_loaded, "test_prospero_compressed_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    @unittest.skipIf(pin3_used, "test_prospero_compressed_using_PIN_traces test: Requires PIN2, but PIN3 is COMPILED.")
    def test_prospero_compressed_using_PIN_traces(self):
        self.prospero_test_template("compressed", NO_DRAMSIM, USE_PIN_TRACES)

    @unittest.skipIf(libz_missing, "test_prospero_compressed_withdramsim_using_PIN_traces test: Requires LIBZ, but LIBZ is not found in build configuration.")
    @unittest.skipIf(not pin_loaded, "test_prospero_compressed_withdramsim_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    @unittest.skipIf(pin3_used, "test_prospero_compressed_withdramsim_using_PIN_traces test: Requires PIN2, but PIN3 is COMPILED.")
    def test_prospero_compressed_withdramsim_using_PIN_traces(self):
        self.prospero_test_template("compressed", WITH_DRAMSIM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_text_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_text_using_PIN_traces(self):
        self.prospero_test_template("text", NO_DRAMSIM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_binary_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_binary_using_PIN_traces(self):
        self.prospero_test_template("binary", NO_DRAMSIM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_text_withdramsim_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_text_withdramsim_using_PIN_traces(self):
        self.prospero_test_template("text", WITH_DRAMSIM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_binary_withdramsim_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_binary_withdramsim_using_PIN_traces(self):
        self.prospero_test_template("binary", WITH_DRAMSIM, USE_PIN_TRACES)

#####

    def prospero_test_template(self, trace_name, with_dramsim, use_pin_traces, testtimeout=240):
        pass
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the paths to the various directories
        self.testProsperoPINTracesDir = "{0}/testProsperoPINTraces".format(tmpdir)
        self.testProsperoTARTracesDir = "{0}/testProsperoTARTraces".format(tmpdir)

        if use_pin_traces:
            propero_trace_dir = self.testProsperoPINTracesDir
        else:
            propero_trace_dir = self.testProsperoTARTracesDir

        # Verify that some trace files exist
        wildcard_filepath = "{0}/*.trace".format(propero_trace_dir)
        trace_files_list = glob.glob(wildcard_filepath)
        self.assertTrue(len(trace_files_list) > 0, "Prosepro - No Trace files found in dir {0}".format(propero_trace_dir))

        # Set the various file paths
        if with_dramsim:
            testDataFileName = ("test_prospero_with_dramsim_{0}".format(trace_name))
            otherargs = '--model-options=\"--TraceType={0} --UseDramSim=yes\"'.format(trace_name)
        else:
            testDataFileName = ("test_prospero_wo_dramsim_{0}".format(trace_name))
            otherargs = '--model-options=\"--TraceType={0} --UseDramSim=no\"'.format(trace_name)

        if use_pin_traces:
            tracetype = "pin"
        else:
            tracetype = "tar"

        sdlfile = "{0}/array/trace-common.py".format(test_path)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}_using_{2}_traces.out".format(outdir, testDataFileName, tracetype)
        errfile = "{0}/{1}_using_{2}_traces.out.err".format(outdir, testDataFileName, tracetype)
        mpioutfiles = "{0}/{1}_using_{2}_traces.out.testfile".format(outdir, testDataFileName, tracetype)

        log_debug("trace_name = {0}".format(trace_name))
        log_debug("testDataFileName = {0}".format(testDataFileName))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, other_args = otherargs,
                     set_cwd=propero_trace_dir, mpi_out_files=mpioutfiles,
                     timeout_sec=testtimeout)

        testing_remove_component_warning_from_file(outfile)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        cmp_result = testing_compare_diff(testDataFileName, outfile, reffile)
        if cmp_result:
            log_debug(" -- Output file {0} is an exact match to Reference File {1}".format(outfile, reffile))
        else:
            wc_out_data = os_wc(outfile, [0, 1])
            log_debug("{0} : wc_out_data = {1}".format(outfile, wc_out_data))
            wc_ref_data = os_wc(reffile, [0, 1])
            log_debug("{0} : wc_ref_data = {1}".format(reffile, wc_ref_data))

            wc_line_word_count_diff = wc_out_data == wc_ref_data
            if wc_line_word_count_diff:
                log_debug("Line Word Count Match\n")
            self.assertTrue(wc_line_word_count_diff, "Line & Word count between file {0} does not match Reference File {1}".format(outfile, reffile))

#######################

    def _setup_prospero_test_dirs(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant code
        log_debug("_setup_prospero_test_dirs() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the paths to the various directories
        self.testProsperoPINTracesDir = "{0}/testProsperoPINTraces".format(tmpdir)
        self.testProsperoTARTracesDir = "{0}/testProsperoTARTraces".format(tmpdir)
        ProsperoElementDir = os.path.abspath("{0}/../".format(test_path))
        sst_elements_parent_dir = os.path.abspath("{0}/../../../../../".format(ProsperoElementDir))
        memHElementsTestsDir = os.path.abspath("{0}/../memHierarchy/tests".format(ProsperoElementDir))

        log_debug("testProsperoPINTraces dir = {0}".format(self.testProsperoPINTracesDir))
        log_debug("testProsperoTARTraces dir = {0}".format(self.testProsperoTARTracesDir))
        log_debug("sst_elements_parent_dir = {0}".format(sst_elements_parent_dir))
        log_debug("memHElementsTestsDir = {0}".format(memHElementsTestsDir))

        # Create a clean version of the testProsperoPINTraces Directory
        if os.path.isdir(self.testProsperoPINTracesDir):
            shutil.rmtree(self.testProsperoPINTracesDir, True)
        os.makedirs(self.testProsperoPINTracesDir)

        # Create a clean version of the testProsperoTARTracesDir Directory
        if os.path.isdir(self.testProsperoTARTracesDir):
            shutil.rmtree(self.testProsperoTARTracesDir, True)
        os.makedirs(self.testProsperoTARTracesDir)

        # Create a simlink of the memH ini files files
        filename = "DDR3_micron_32M_8B_x4_sg125.ini"
        os_symlink_file(memHElementsTestsDir, self.testProsperoPINTracesDir, filename)
        os_symlink_file(memHElementsTestsDir, self.testProsperoTARTracesDir, filename)

        filename = "system.ini"
        os_symlink_file(memHElementsTestsDir, self.testProsperoPINTracesDir, filename)
        os_symlink_file(memHElementsTestsDir, self.testProsperoTARTracesDir, filename)

####

    def _create_prospero_PIN_trace_files(self):
        # Now if PIN IS AVAILABLE, Create the PIN Trace Files
        if not testing_is_PIN_loaded():
            log_debug("_create_prospero_PIN_trace_files() - PIN Not available, we will not build PIN traces")
            return

        log_debug("_create_prospero_PIN_trace_files() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Create a simlink of the Ariel files
        sourcedir = "{0}/array".format(test_path)
        targetdir = self.testProsperoPINTracesDir

        # Create a simlink of each file in the source dir to the target dir
        for f in os.listdir(sourcedir):
            os_symlink_file(sourcedir, targetdir, f)

        # Now build the array application
        cmd = "make"
        rtn = OSCommand(cmd, set_cwd=targetdir).run()
        log_debug("Prospero tests/array Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "array.c failed to compile")

        # Make sure we have access to the sst-prospero-trace binary
        elem_bin_dir = sstsimulator_conf_get_value_str("SST_ELEMENT_LIBRARY", "SST_ELEMENT_LIBRARY_BINDIR", "BINDIR_UNDEFINED")
        log_debug("Elements bin_dir = {0}".format(elem_bin_dir))
        filepath_sst_prospero_trace_app = "{0}/sst-prospero-trace".format(elem_bin_dir)
        if os.path.isfile(filepath_sst_prospero_trace_app):

            # Now build the text traces
            cmd = "{0} -ifeellucky -t 1 -f text -o sstprospero -- ./array".format(filepath_sst_prospero_trace_app)
            log_debug("Prospero text Traces build cmd = {0}".format(cmd))
            rtn = OSCommand(cmd, set_cwd=targetdir).run()
            log_debug("Prospero build text Traces result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
            self.assertTrue(rtn.result() == 0, "Text Traces failed to compile")

            # Now build the binary traces
            cmd = "{0} -ifeellucky -t 1 -f binary -o sstprospero -- ./array".format(filepath_sst_prospero_trace_app)
            log_debug("Prospero binary Traces build cmd = {0}".format(cmd))
            rtn = OSCommand(cmd, set_cwd=targetdir).run()
            log_debug("Prospero build binary Traces result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
            self.assertTrue(rtn.result() == 0, "Binary Traces failed to compile")

            # Now build the Compressed traces (ONLY ON PIN2)
            if testing_is_PIN2_used():
                cmd = "{0} -ifeellucky -t 1 -f compressed -o sstprospero -- ./array".format(filepath_sst_prospero_trace_app)
                log_debug("Prospero compressed Traces build cmd = {0}".format(cmd))
                rtn = OSCommand(cmd, set_cwd=targetdir).run()
                log_debug("Prospero build compressed Traces result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
                self.assertTrue(rtn.result() == 0, "Compressed Traces failed to compile")

####

    def _download_prospero_TAR_trace_files(self):
        log_debug("_download_prospero_TAR_trace_files() Running")
        # Now download the PIN TRACE FILES

        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # wget a test file tar.gz
        testfile = "Prospero-trace-files.tar.gz"
        fileurl = "https://github.com/sstsimulator/sst-downloads/releases/download/TestFiles/{0}".format(testfile)
        self.assertTrue(os_wget(fileurl, self.testProsperoTARTracesDir), "Failed to download {0}".format(testfile))

        # Extract the test file
        filename = "{0}/{1}".format(self.testProsperoTARTracesDir, testfile)
        os_extract_tar(filename, self.testProsperoTARTracesDir)




