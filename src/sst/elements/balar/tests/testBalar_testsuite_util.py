# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import test_engine_globals

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
        print("Initializing balar test files...")
        class_inst._setupbalarTestFiles()
        # Unset 
        log_debug("Unsetting env vars: VANADIS_EXE VANADIS_ARGS")
        os.environ["VANADIS_EXE"] = ""
        os.environ["VANADIS_ARGS"] = ""
        module_init = 1

    module_sema.release()

################################################################################

class BalarTestCase(SSTTestCase):

    def setUp(self):
        super().setUp()
        self._setupbalarVarEnv()
        initializeTestModule_SingleInstance(self)

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super().tearDown()

#####
    # missing_cuda_root = os.getenv("CUDA_ROOT") == None
    missing_cuda_install_path = os.getenv("CUDA_INSTALL_PATH") == None
    missing_gpgpusim_root = os.getenv("GPGPUSIM_ROOT") == None
    missing_riscv_cuda_clang_path = os.getenv("LLVM_INSTALL_PATH") == None
    missing_riscv_gcc_path = os.getenv("RISCV_TOOLCHAIN_INSTALL_PATH") == None
    missing_gpu_app_collection_root = os.getenv("GPUAPPS_ROOT") == None

    def compose_decorators(*decs):
        def composed(f):
            for dec in reversed(decs):
                f = dec(f)
            return f
        return composed
    
    def balar_basic_unittest(test):
        def wrapper(*args):
            self = args[0]
            return self.compose_decorators(
                unittest.skipIf(self.missing_cuda_install_path, "test_balar_runvecadd test: Requires missing environment variable CUDA_INSTALL_PATH."),
                unittest.skipIf(self.missing_gpgpusim_root, "test_balar_runvecadd test: Requires missing environment variable GPGPUSIM_ROOT."),
                test(*args)
            )
        return wrapper

    def balar_gpuapp_unittest(test):
        def wrapper(*args):
            self = args[0]
            return self.compose_decorators(
                self.balar_basic_unittest,
                unittest.skipIf(self.missing_gpu_app_collection_root, "test_balar_clang test: Requires missing environment variable GPUAPPS_ROOT."),
                test(*args)
            )
        return wrapper

####

    def balar_testcpu_template(self, testcase, testtimeout=400):
        """Balar testcase template with trace-driven mode

        Args:
            testcase (str): testcase name
            testtimeout (int, optional): testcase timeout. Defaults to 400.
        """
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
        sdlfile = "{0}/testBalar-testcpu.py".format(test_path)
        vecAddBinary = "{0}/vectorAdd".format(self.testbalarVectorAddDir)
        vecAddTrace = "{0}/cuda_calls.trace".format(self.testbalarVectorAddDir)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))
        log_debug("stats file = {0}".format(statsfile))
        log_debug("testbalarDir = {0}".format(self.testbalarDir))

        gpuMemCfgfile = "{0}/gpu-v100-mem.cfg".format(self.testbalarDir)
        otherargs = '--model-options=\"-c {0} -s {1} -x {2} -t {3}"'.format(gpuMemCfgfile, statsfile, vecAddBinary, vecAddTrace)

        # Run SST
        # Unset BALAR_CUDA_EXE_PATH so that we will use the path
        # in the __cudaRegisterFatbin function call
        cmd = self.run_sst(sdlfile, outfile, errfile, set_cwd=self.testbalarDir,
                     other_args=otherargs,
                     timeout_sec=testtimeout)
        log_debug("cmd = {0}".format(cmd))

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

    def balar_vanadis_template(self, testcase, testtimeout=400):
        """Balar testcase template with vanadis with explicit CUDA api calls

        Args:
            testcase (str): testcase name
            testtimeout (int, optional): testcase timeout. Defaults to 400.
        """
        # Have to test for NVCC_PATH inside of the test as decorated skips happen
        # before init of testsuite
        missing_nvcc_path = os.getenv("NVCC_PATH") == None
        self.assertFalse(missing_nvcc_path, "balar_vanadis_template test: Requires missing environment variable NVCC_PATH.")

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.balarElementDir = os.path.abspath("{0}/../".format(test_path))
        self.balarElementVanadisHandshakeTestDir = os.path.abspath("{0}/vanadisHandshake".format(test_path))
        self.testbalarDir = "{0}/test_balar".format(tmpdir)
        self.testbalarVanadisHandshakeDir = "{0}/vanadisHandshake".format(self.testbalarDir)

        # Set the various file paths
        testDataFileName="test_gpgpu_{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        statsfile = "{0}/{1}.stats_out".format(outdir, testDataFileName)
        sdlfile = "{0}/testBalar-vanadis.py".format(test_path)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))
        log_debug("stats file = {0}".format(statsfile))
        log_debug("testbalarDir = {0}".format(self.testbalarDir))

        gpuMemCfgfile = "{0}/gpu-v100-mem.cfg".format(self.testbalarDir)
        otherargs = '--model-options=\"-c {0} -s {1} --vanadis-binary {2}"'.format(gpuMemCfgfile, statsfile, "./vanadisHandshake/vanadisHandshake")

        # Run SST
        os.environ["VANADIS_ISA"] = "RISCV64"
        cmd = self.run_sst(sdlfile, outfile, errfile, set_cwd=self.testbalarDir,
                           other_args=otherargs,
                           timeout_sec=testtimeout)
        log_debug("cmd = {0}".format(cmd))

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

    def balar_vanadis_clang_template(self, testcase, testtimeout=600):
        """Balar testcase template with vanadis with executable compiled from Clang-CUDA

        Args:
            testcase (str): testcase name
            testtimeout (int, optional): testcase timeout. Defaults to 400.
        """

        # Get necessary executable paths
        riscv_cuda_clang_path = os.getenv("LLVM_INSTALL_PATH")
        riscv_gcc_path = os.getenv("RISCV_TOOLCHAIN_INSTALL_PATH")
        cuda_version_num = os.getenv("CUDA_VERSION")
        gpu_app_collection_root = os.getenv("GPUAPPS_ROOT")

        # Test cases dispatch dict
        # Tag: [EXE_PATH, DATA_DIR, EXE_args]
        testcases = {
            "helloworld": ["./vanadisLLVMRISCV/helloworld", "", ""],
            "vecadd": ["./vanadisLLVMRISCV/vecadd", "", ""],
            "simpleStreams": ["./vanadisLLVMRISCV/simpleStreams", "", ""],
            # Rodinia 2.0
            ## Backprop
            "rodinia-2.0-backprop-short": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/backprop-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/backprop-rodinia-2.0-ft", f"256"],
            "rodinia-2.0-backprop-1024": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/backprop-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/backprop-rodinia-2.0-ft", f"1024"],
            "rodinia-2.0-backprop-2048": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/backprop-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/backprop-rodinia-2.0-ft", f"2048"],
            "rodinia-2.0-backprop-4096": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/backprop-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/backprop-rodinia-2.0-ft", f"4096"],
            "rodinia-2.0-backprop-8192": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/backprop-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/backprop-rodinia-2.0-ft", f"8192"],

            ## BFS
            "rodinia-2.0-bfs-SampleGraph": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/bfs-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/bfs-rodinia-2.0-ft/", "./data/SampleGraph.txt"],
            "rodinia-2.0-bfs-graph4096": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/bfs-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/bfs-rodinia-2.0-ft/", "./data/graph4096.txt"],
            "rodinia-2.0-bfs-graph65536": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/bfs-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/bfs-rodinia-2.0-ft/", "./data/graph65536.txt"],

            ## Heartwall
            "rodinia-2.0-heartwall-1": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/heartwall-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/heartwall-rodinia-2.0-ft/", f"./data/test.avi 1 ./data/result-1.txt"],
            
            ## Hotspot
            "rodinia-2.0-hotspot-30-6-40": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/hotspot-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/hotspot-rodinia-2.0-ft/", f"30 6 40 ./data/result_30_6_40.txt ./data/temp.dat ./data/power.dat"],

            ## LUD
            "rodinia-2.0-lud-64": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/lud-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/lud-rodinia-2.0-ft", f"-i ./data/64.dat"],
            "rodinia-2.0-lud-256": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/lud-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/lud-rodinia-2.0-ft", f"-i ./data/256.dat"],
            "rodinia-2.0-lud-512": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/lud-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/lud-rodinia-2.0-ft", f"-i ./data/512.dat"],
            "rodinia-2.0-lud-2048": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/lud-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/lud-rodinia-2.0-ft", f"-i ./data/2048.dat"],

            ## NN
            "rodinia-2.0-nn-4-3-30-90": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/nn-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/nn-rodinia-2.0-ft", f"./data/filelist_4 3 30 90 ./data/filelist_4_3_30_90-result.txt"],

            ## NW
            "rodinia-2.0-nw-128-10": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/nw-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/nw-rodinia-2.0-ft", f"128 10"],
            "rodinia-2.0-nw-256-10": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/nw-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/nw-rodinia-2.0-ft", f"256 10"],
            "rodinia-2.0-nw-512-10": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/nw-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/nw-rodinia-2.0-ft", f"512 10"],

            ## Pathfinder
            "rodinia-2.0-pathfinder-1000-20-5": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/pathfinder-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/pathfinder-rodinia-2.0-ft", f"1000 20 5 ./data/result_1000_20_5.txt"],

            ## Srad_v2
            "rodinia-2.0-srad_v2-128x128": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/srad_v2-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/srad_v2-rodinia-2.0-ft", f"./data/matrix128x128.txt 0 127 0 127 .5 2 ./data/result_matrix128x128_1_150_1_100_.5_2.txt"],
            
            ## Streamcluster
            "rodinia-2.0-streamcluster-3_6_16_1024_1024_100_none_1": [f"{gpu_app_collection_root}/bin/{cuda_version_num}/release/streamcluster-rodinia-2.0-ft", f"{gpu_app_collection_root}/data_dirs/cuda/rodinia/2.0-ft/streamcluster-rodinia-2.0-ft", f"3 6 16 1024 1024 100 none output.txt 1 ./data/result_3_6_16_1024_1024_100_none_1.txt"]
        }

        # Get gpu apps executable and args
        exe, data_dir, args = testcases[testcase]

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Create testcase dir and link its data dir from
        # gpu app collection
        testcase_dir = self.testbalarDir
        if data_dir != "":
            testcase_dir = f"{self.testbalarDir}/{testcase}"
            testcase_data_dir = f"{testcase_dir}/data"
            os.makedirs(testcase_dir)
            os_symlink_dir(f"{data_dir}/data", testcase_data_dir)
            
            # Also link config files as we are changing the cwd to here
            os_symlink_file(self.testbalarDir, testcase_dir, "gpu-v100-mem.cfg")
            os_symlink_file(self.testbalarDir, testcase_dir, "gpgpusim.config")
            os_symlink_file(self.testbalarDir, testcase_dir, "utils.py")
            os_symlink_file(self.testbalarDir, testcase_dir, "balarBlock.py")
            os_symlink_file(self.testbalarDir, testcase_dir, "memory.py")
            os_symlink_file(self.testbalarDir, testcase_dir, "vanadisBlock.py")
            os_symlink_file(self.testbalarDir, testcase_dir, "vanadisOS.py")

        # Set the various file paths
        testDataFileName="test_gpgpu_{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        statsfile = "{0}/{1}.stats_out".format(outdir, testDataFileName)
        sdlfile = "{0}/testBalar-vanadis.py".format(test_path)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))
        log_debug("stats file = {0}".format(statsfile))
        log_debug("testbalarDir = {0}".format(self.testbalarDir))

        gpuMemCfgfile = "{0}/gpu-v100-mem.cfg".format(self.testbalarDir)
        otherargs = '--model-options=\"-c {0} -s {1} --vanadis-binary {2} --vanadis-args=\\"{3}\\" --cuda-binary {4}\"'.format(gpuMemCfgfile, statsfile, exe, args, exe)

        # Run SST
        os.environ["VANADIS_ISA"] = "RISCV64"
        cmd = self.run_sst(sdlfile, outfile, errfile, set_cwd=testcase_dir,
                           other_args=otherargs,
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
        
        nthreads = test_engine_globals.TESTENGINE_THREADLIMIT
        log_debug(f"_setupbalarTestFiles() Running, make with {nthreads}")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Create a clean version of the testbalar Directory
        if os.path.isdir(self.testbalarDir):
            shutil.rmtree(self.testbalarDir, True)
        os.makedirs(self.testbalarDir)
        os.makedirs(self.testbalarVectorAddDir)
        os.makedirs(self.testbalarVanadisHandshakeDir)
        os.makedirs(self.testbalarLLVMVanadisDir)

        # Create a simlink of required test files balar/tests directory
        os_symlink_file(test_path, self.testbalarDir, "gpu-v100-mem.cfg")
        os_symlink_file(test_path, self.testbalarDir, "gpgpusim.config")
        os_symlink_file(test_path, self.testbalarDir, "utils.py")
        os_symlink_file(test_path, self.testbalarDir, "balarBlock.py")
        os_symlink_file(test_path, self.testbalarDir, "memory.py")
        os_symlink_file(test_path, self.testbalarDir, "vanadisBlock.py")
        os_symlink_file(test_path, self.testbalarDir, "vanadisOS.py")
        # Copy the shared packet definition files from balar src
        os_symlink_file(self.balarElementDir, tmpdir, "balar_packet.h")
        os_symlink_file(self.balarElementDir, tmpdir, "balar_consts.h")

        # Create a simlink of each file in the balar/tests/vectorAdd directory
        for f in os.listdir(self.balarElementVectorAddTestDir):
            os_symlink_file(self.balarElementVectorAddTestDir, self.testbalarVectorAddDir, f)
        
        # Create a simlink of each file in the balar/tests/vanadisHandshake directory
        for f in os.listdir(self.balarElementVanadisHandshakeTestDir):
            os_symlink_file(self.balarElementVanadisHandshakeTestDir, self.testbalarVanadisHandshakeDir, f)

        # Create a simlink of each file in the balar/tests/vanadisLLVMRISCV directory
        for f in os.listdir(self.balarElementLLVMVanadisTestDir):
            os_symlink_file(self.balarElementLLVMVanadisTestDir, self.testbalarLLVMVanadisDir, f)
        
        # Now build the vectorAdd example
        cmd = "make"
        rtn = OSCommand(cmd, set_cwd=self.testbalarVectorAddDir).run()
        log_debug("Balar vectorAdd Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "vecAdd.cu failed to compile")

        # Build vanadishandshake
        cmd = "make"
        rtn = OSCommand(cmd, set_cwd=self.testbalarVanadisHandshakeDir).run()
        log_debug("Balar vanadisHandshake Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "vanadisHandshake.c failed to compile")

        # Build vanadisLLVMRISCV, contains helloworld, vecadd, and the custom CUDA lib
        cmd = "make"
        rtn = OSCommand(cmd, set_cwd=self.testbalarLLVMVanadisDir).run()
        log_debug("Balar vanadisLLVM Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "vanadisLLVM executables and lib failed to compile")

        # Let GPU app knows we are building for SST integration
        if not self.missing_gpu_app_collection_root:
            # Compile rodinia 2.0
            # Let rodinia's makefile aware for the custom cuda lib in our temp directory
            os.environ["SST_CUSTOM_CUDA_LIB_PATH"] = self.testbalarLLVMVanadisDir

            # Build rodinia
            # Use the same amount of concurrent run to compile rodinia
            cmd_compile = f"make rodinia_2.0-ft -j{nthreads} -C ./src"
            rtn = OSCommand(cmd_compile, set_cwd=os.environ["GPUAPPS_ROOT"]).run()
            log_debug("Balar vanadisLLVM GPU App rodinia 2.0 Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
            self.assertTrue(rtn.result() == 0, "vanadisLLVM GPU app rodinia 2.0 benchmark failed to compile")

            # Pull data
            cmd_pulldata = "make data -C ./src"
            rtn = OSCommand(cmd_pulldata, set_cwd=os.environ["GPUAPPS_ROOT"]).run()
            log_debug("Balar vanadisLLVM GPU App Data pull result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
            self.assertTrue(rtn.result() == 0, "vanadisLLVM GPU app benchmark failed to pull data")
        else:
            log_debug("Missing GPU App collection, skipping compilation for it")

    def _setupbalarVarEnv(self):
        # Figure out path to NVCC and set NVCC_PATH in the OS ENV
        # NOTE: In sst-gpgpusim, the setup_environment script sets the NVCC_PATH
        #       but does not export it to to the OS Env.  So we need to do it here
        log_debug("_setupbalarVarEnv() Running")

        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.balarElementDir = os.path.abspath("{0}/../".format(test_path))
        self.balarElementVectorAddTestDir = os.path.abspath("{0}/vectorAdd".format(test_path))
        self.balarElementVanadisHandshakeTestDir = os.path.abspath("{0}/vanadisHandshake".format(test_path))
        self.balarElementLLVMVanadisTestDir = os.path.abspath("{0}/vanadisLLVMRISCV".format(test_path))
        self.testbalarDir = "{0}/test_balar".format(tmpdir)
        self.testbalarVectorAddDir = "{0}/vectorAdd".format(self.testbalarDir)
        self.testbalarVanadisHandshakeDir = "{0}/vanadisHandshake".format(self.testbalarDir)
        self.testbalarLLVMVanadisDir = "{0}/vanadisLLVMRISCV".format(self.testbalarDir)

        cmd = "which nvcc"
        rtn = OSCommand(cmd).run()
        log_debug("which nvcc result = {0}; output = {1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "'which nvcc' command failed")
        os.environ["NVCC_PATH"] = rtn.output()

        # Set the CUDA version here as well by parsing the nvcc version string
        cmd = "nvcc --version"
        rtn = OSCommand(cmd).run()
        log_debug("CUDA version string: result = {0}; output = {1}".format(rtn.result(), rtn.output()))
        # Parse version string
        version_string = rtn.output()
        tmp = version_string.split("release ")[1]
        version_num = tmp.split(",")[0].strip()
        os.environ["CUDA_VERSION"] = version_num
