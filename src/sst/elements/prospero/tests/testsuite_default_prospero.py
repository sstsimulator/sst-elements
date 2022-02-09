# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os
import glob

USE_PIN_TRACES = True
USE_TAR_TRACES = False
WITH_TIMINGDRAM = True
NO_TIMINGDRAM = False

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
        self.prospero_test_template("compressed", NO_TIMINGDRAM, USE_TAR_TRACES)

    @unittest.skipIf(libz_missing, "test_prospero_compressed_withtimingdram_using_TAR_traces test: Requires LIBZ, but LIBZ is not found in build configuration.")
    def test_prospero_compressed_withtimingdram_using_TAR_traces(self):
        self.prospero_test_template("compressed", WITH_TIMINGDRAM, USE_TAR_TRACES)

    def test_prospero_text_using_TAR_traces(self):
        self.prospero_test_template("text", NO_TIMINGDRAM, USE_TAR_TRACES)

    def test_prospero_binary_using_TAR_traces(self):
        self.prospero_test_template("binary", NO_TIMINGDRAM, USE_TAR_TRACES)

    def test_prospero_text_withtimingdram_using_TAR_traces(self):
        self.prospero_test_template("text", WITH_TIMINGDRAM, USE_TAR_TRACES)

    def test_prospero_binary_withtimingdram_using_TAR_traces(self):
        self.prospero_test_template("binary", WITH_TIMINGDRAM, USE_TAR_TRACES)

    @unittest.skipIf(libz_missing, "test_prospero_compressed_using_PIN_traces test: Requires LIBZ, but LIBZ is not found in build configuration.")
    @unittest.skipIf(not pin_loaded, "test_prospero_compressed_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    @unittest.skipIf(pin3_used, "test_prospero_compressed_using_PIN_traces test: Requires PIN2, but PIN3 is COMPILED.")
    def test_prospero_compressed_using_PIN_traces(self):
        self.prospero_test_template("compressed", NO_TIMINGDRAM, USE_PIN_TRACES)

    @unittest.skipIf(libz_missing, "test_prospero_compressed_withtimingdram_using_PIN_traces test: Requires LIBZ, but LIBZ is not found in build configuration.")
    @unittest.skipIf(not pin_loaded, "test_prospero_compressed_withtimingdram_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    @unittest.skipIf(pin3_used, "test_prospero_compressed_withtimingdram_using_PIN_traces test: Requires PIN2, but PIN3 is COMPILED.")
    def test_prospero_compressed_withtimingdram_using_PIN_traces(self):
        self.prospero_test_template("compressed", WITH_TIMINGDRAM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_text_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_text_using_PIN_traces(self):
        self.prospero_test_template("text", NO_TIMINGDRAM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_binary_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_binary_using_PIN_traces(self):
        self.prospero_test_template("binary", NO_TIMINGDRAM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_text_withtimingdram_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_text_withtimingdram_using_PIN_traces(self):
        self.prospero_test_template("text", WITH_TIMINGDRAM, USE_PIN_TRACES)

    @unittest.skipIf(not pin_loaded, "test_prospero_binary_withtimingdram_using_PIN_traces: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_prospero_binary_withtimingdram_using_PIN_traces(self):
        self.prospero_test_template("binary", WITH_TIMINGDRAM, USE_PIN_TRACES)

#####

    def prospero_test_template(self, trace_name, with_timingdram, use_pin_traces, testtimeout=240):
        pass
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the paths to the various directories
        self.testProsperoPINTracesDir = "{0}/testProsperoPINTraces".format(tmpdir)
        self.testProsperoTARTracesDir = "{0}/testProsperoTARTraces".format(tmpdir)

        if use_pin_traces:
            prospero_trace_dir = self.testProsperoPINTracesDir
        else:
            prospero_trace_dir = self.testProsperoTARTracesDir

        # Verify that some trace files exist
        wildcard_filepath = "{0}/*.trace".format(prospero_trace_dir)
        trace_files_list = glob.glob(wildcard_filepath)
        self.assertTrue(len(trace_files_list) > 0, "Prospero - No Trace files found in dir {0}".format(prospero_trace_dir))

        # Set the various file paths
        if with_timingdram:
            testDataFileName = ("test_prospero_with_timingdram_{0}".format(trace_name))
            otherargs = '--model-options=\"--TraceType={0} --UseTimingDram=yes --TraceDir={1}\"'.format(trace_name, prospero_trace_dir)
        else:
            testDataFileName = ("test_prospero_wo_timingdram_{0}".format(trace_name))
            otherargs = '--model-options=\"--TraceType={0} --UseTimingDram=no --TraceDir={1}\"'.format(trace_name, prospero_trace_dir)

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
                     set_cwd=prospero_trace_dir, mpi_out_files=mpioutfiles,
                     timeout_sec=testtimeout)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID
        
        ### Check for success ###
        
        # Lines to ignore during diff
        ## This is generated by SST when the number of ranks/threads > # of components
        ignore_lines = ["WARNING: No components are assigned to"]
        ## These are warnings/info generated by SST/memH in debug mode
        ignore_lines.append("Notice: memory controller's region is larger than the backend's mem_size")
        ignore_lines.append("Region: start=")

        filesAreTheSame, statDiffs, othDiffs = testing_stat_output_diff(outfile, reffile, ignore_lines, {}, True)
        
        # Perform the tests
        
        if filesAreTheSame:
            log_debug(" -- Output file {0} passed check against the Reference File {1}".format(outfile, reffile))
        elif use_pin_traces: ## PIN traces are generated dynamically and may diff, but the line count should match
            # Use processed diffs so that ignore lines are still ignored
            stat_lc = sum(1 for x in statDiffs if x[0] == "<")
            oth_lc = sum(1 for x  in othDiffs if x[0] == "<")
            if stat_lc*2 == len(statDiffs) and oth_lc*2 == len(othDiffs):
                log_debug(" -- Output file {0} pass line count check against the Reference File {1}".format(outfile, reffile))
            else:
                diffdata = self._prettyPrintDiffs(statDiffs, othDiffs)
                log_failure(diffdata)
                self.assertTrue(filesAreTheSame, "Output file {0} does not pass line count check against the Reference File {1} ".format(outfile, reffile))
                
        else:
            diffdata = self._prettyPrintDiffs(statDiffs, othDiffs)
            log_failure(diffdata)
            self.assertTrue(filesAreTheSame, "Output file {0} does not pass check against the Reference File {1} ".format(outfile, reffile))


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


    def _prettyPrintDiffs(self, stat_diff, oth_diff):
        out = ""
        if len(stat_diff) != 0:
            out = "Statistic diffs:\n"
            for x in stat_diff:
                out += (x[0] + " " + ",".join(str(y) for y in x[1:]) + "\n")
        
        if len(oth_diff) != 0:
            out += "Non-statistic diffs:\n"
            for x in oth_diff:
                out += x[0] + " " + x[1] + "\n"

        return out


