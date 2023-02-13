# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

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
        class_inst._setupbalarTestFiles()
        module_init = 1

    module_sema.release()

################################################################################

class testcase_balar(SSTTestCase):

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
    pin_loaded = testing_is_PIN_loaded()
    pin3_used = testing_is_PIN3_used()
    missing_cuda_root = os.getenv("CUDA_ROOT") == None
    missing_cuda_install_path = os.getenv("CUDA_INSTALL_PATH") == None
    missing_gpgpusim_root = os.getenv("GPGPUSIM_ROOT") == None
    found_mem_pools_config = sst_core_config_include_file_get_value_str("USE_MEMPOOL", default="", disable_warning=True) != ""
    found_mpi_config = sst_core_config_include_file_get_value_str("SST_CONFIG_HAVE_MPI", default="", disable_warning=True) != ""

    @unittest.skipIf(not pin_loaded, "test_balar_runvecadd: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    @unittest.skipIf(pin3_used, "test_balar_runvecadd test: Requires PIN2, but PIN3 is COMPILED.")
    @unittest.skipIf(missing_cuda_root, "test_balar_runvecadd test: Requires missing environment variable CUDA_ROOT.")
    @unittest.skipIf(missing_cuda_install_path, "test_balar_runvecadd test: Requires missing environment variable CUDA_INSTALL_PATH.")
    @unittest.skipIf(missing_gpgpusim_root, "test_balar_runvecadd test: Requires missing environment variable GPGPUSIM_ROOT.")
    @unittest.skipIf(found_mem_pools_config, "test_balar_runvecadd test: Found mem-pools configured in core, test requires core to be built using --disable-mem-pools.")
    @unittest.skipIf(found_mpi_config, "test_balar_runvecadd test: Found mpi configured in core, test requires core to be built using --disable-mpi.")

    def test_balar_runvecadd(self):
        self.balar_test_template("vectorAdd")

####

    def balar_test_template(self, testcase, testtimeout=400):
        # Have to test for NVCC_PATH inside of the test as decorated skips happen
        # before init of testsuite
        missing_nvcc_path = os.getenv("NVCC_PATH") == None
        self.assertFalse(missing_nvcc_path, "test_balar_runvecadd test: Requires missing environment variable NVCC_PATH.")

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.balarElementDir = os.path.abspath("{0}/../".format(test_path))
        self.balarElementVectorAddTestDir = os.path.abspath("{0}/vectorAdd".format(test_path))
        self.testbalarDir = "{0}/test_balar".format(tmpdir)
        self.testbalarVectorAddDir = "{0}/vectorAdd".format(self.testbalarDir)

        # Set the various file paths
        testDataFileName="test_gpgpu_{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        statsfile = "{0}/{1}.stats_out".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/cuda-test.py".format(test_path, testcase)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))
        log_debug("stats file = {0}".format(statsfile))
        log_debug("testbalarDir = {0}".format(self.testbalarDir))

        arielcfgfile = "{0}/ariel-gpu-v100.cfg".format(self.testbalarDir)
        otherargs = '--model-options=\"-c {0} -s {1}"'.format(arielcfgfile, statsfile)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, set_cwd=self.testbalarDir,
                     mpi_out_files=mpioutfiles, other_args=otherargs,
                     timeout_sec=testtimeout)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        self.assertTrue(os_test_file(statsfile, "-e"), "balar test {0} is missing the stats file {1}".format(testDataFileName, statsfile))

        num_stat_lines  = int(os_wc(statsfile, [0]))
        log_debug("{0} : num_stat_lines = {1}".format(outfile, num_stat_lines))
        num_ref_lines = int(os_wc(reffile, [0]))
        log_debug("{0} : num_ref_lines = {1}".format(reffile, num_ref_lines))

        line_count_diff = abs(num_ref_lines - num_stat_lines)
        log_debug("Line Count diff = {0}".format(line_count_diff))

        if line_count_diff > 15:
            self.assertFalse(line_count_diff > 15, "Line count between Stats file {0} does not match Reference File {1}; They contain {2} different lines".format(statsfile, reffile, line_count_diff))

#####

    def _setupbalarTestFiles(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setupbalarTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.balarElementDir = os.path.abspath("{0}/../".format(test_path))
        self.balarElementVectorAddTestDir = os.path.abspath("{0}/vectorAdd".format(test_path))
        self.testbalarDir = "{0}/test_balar".format(tmpdir)
        self.testbalarVectorAddDir = "{0}/vectorAdd".format(self.testbalarDir)

        # Create a clean version of the testbalar Directory
        if os.path.isdir(self.testbalarDir):
            shutil.rmtree(self.testbalarDir, True)
        os.makedirs(self.testbalarDir)
        os.makedirs(self.testbalarVectorAddDir)

        # Create a simlink of required test files balar/tests directory
        os_symlink_file(test_path, self.testbalarDir, "ariel-gpu-v100.cfg")
        os_symlink_file(test_path, self.testbalarDir, "gpgpusim.config")
        os_symlink_file(test_path, self.testbalarDir, "utils.py")

        # Create a simlink of each file in the balar/tests/vectorAdd directory
        for f in os.listdir(self.balarElementVectorAddTestDir):
            os_symlink_file(self.balarElementVectorAddTestDir, self.testbalarVectorAddDir, f)

        # Now build the vectorAdd example
        cmd = "make"
        rtn = OSCommand(cmd, set_cwd=self.testbalarVectorAddDir).run()
        log_debug("Balar vectorAdd Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "vecAdd.cu failed to compile")

        # Figure out path to NVCC and set NVCC_PATH in the OS ENV
        # NOTE: In sst-gpgpusim, the setup_environment script sets the NVCC_PATH
        #       but does not export it to to the OS Env.  So we need to do it here
        cmd = "which nvcc"
        rtn = OSCommand(cmd).run()
        log_debug("which nvcc result = {0}; output = {1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "'which nvcc' command failed")
        os.environ["NVCC_PATH"] = rtn.output()
