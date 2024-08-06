# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os
import inspect
import subprocess

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
# Functions to support parsing the output of the MPI tests

def get_reduce_string(rank, ranks, size=1024):
        return [f"Rank {rank} partial sum is {sum(range(int(rank*(size/ranks)), int((rank+1)*(size/ranks))))}, total sum is {sum(range(size))}\n"]

def get_hello_string(rank, ranks, tracerank, threads):
    if rank == tracerank:
        return [f"Hello from rank {rank} of {ranks}, thread {i}! (Launched by pin)\n" for i in range(threads)]
    else:
        return [f"Hello from rank {rank} of {ranks}, thread {i}!\n" for i in range(threads)]
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

    # Test that the output contains the specified line. Because the programs are
    # Multithreaded, we cannot know ahead of time which line will match. The
    # programs use #pragma critical so we expect that the output from each thread
    # will be on its own line.
    def file_contains(self, filename, strings):
        with open(filename, 'r') as file:
            lines = file.readlines()
            for s in strings:
                self.assertTrue(s in lines, "Output {0} does not contain expected line {1}".format(filename, s))

    # Test that the stats file `filename` has a non-zero value for statistics `stat`.
    def assert_nonzero_stat(self, filename, stat):
        with open(filename, 'r') as file:
            lines = file.readlines()
            for ln in lines:
                l = ln.split(' ')
                if l[0] == stat:
                    stat_value = int(l[12].split(';')[0])
                    self.assertTrue(stat_value > 0, f"Statistics file `{filename}` did not have a positive value for stat `{stat}`. Line was:\n\t{ln}")

    pin_loaded = testing_is_PIN_loaded()
    pin_error_msg = "Ariel: Requires PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist."

    multi_rank = testing_check_get_num_ranks() > 1
    multi_rank_error_msg = "Ariel: Ariel MPI tests are not compatible with multi-rank sst runs."

    using_osx = host_os_is_osx()
    osx_error_msg = "Ariel: OpenMP is not supported on macOS"

    # TODO: This is hacky. What is the correct way to get the test script location?
    testsuite_dir = os.path.dirname(__file__)
    mpilauncher_exists = os.path.isfile(testsuite_dir + '/../mpi/mpilauncher')
    mpi_error_msg = f"Ariel: The mpilauncher executable was not found"

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_01(self):
        self.ariel_Template(threads=1, ranks=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_02(self):
        self.ariel_Template(threads=1, ranks=2)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_03(self):
        self.ariel_Template(threads=2, ranks=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_04(self):
        self.ariel_Template(threads=1, ranks=2, tracerank=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_05(self):
        self.ariel_Template(threads=2, ranks=3, tracerank=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_06(self):
        self.ariel_Template(threads=2, ranks=2)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_reduce_01(self):
        self.ariel_Template(threads=1, ranks=1, program="reduce")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_reduce_02(self):
        self.ariel_Template(threads=2, ranks=2, program="reduce")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_reduce_03(self):
        self.ariel_Template(threads=2, ranks=4, program="reduce", tracerank=1)

    def ariel_Template(self, threads, ranks, program="hello", tracerank=0, testtimeout=60, size=8000):
        # Set the paths to the various directories
        testcase = inspect.stack()[1][3] # name the test after the calling function

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set paths
        ArielElementDir = os.path.abspath("{0}/../".format(test_path))
        ArielElementAPIDir = "{0}/api".format(ArielElementDir)
        ArielElementMPIDir = "{0}/mpi".format(ArielElementDir)
        ArielElementTestMPIDir = "{0}/tests/testMPI".format(ArielElementDir)

        libpath = os.environ.get("LD_LIBRARY_PATH")
        if libpath:
            os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir + ":" + libpath
        else:
            os.environ["LD_LIBRARY_PATH"] = ArielElementAPIDir

        # Set the various file paths
        testDataFileName=("{0}".format(testcase))

        sdlfile = "{0}/test-mpi.py".format(ArielElementTestMPIDir)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        statfile = f"{ArielElementTestMPIDir}/stats.csv"
        program_output = f"{tmpdir}/ariel_testmpi_{testcase}.out"
        other_args = f'--model-options="{program} -o {program_output} -r {ranks} -t {threads} -a {tracerank} -s {size}"'

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=ArielElementTestMPIDir,
                     mpi_out_files=mpioutfiles, timeout_sec=testtimeout, other_args=other_args)

        # Each rank will have its own output file
        # We will examine all of them.

        # These programs are designed to output a separate file for each rank,
        # and append the rank id to the argument
        program_output_files = [f"{program_output}_{i}" for i in range(ranks)]

        # Look for the word "FATAL" in the sst output file
        cmd = 'grep "FATAL" {0} '.format(outfile)
        grep_result = os.system(cmd) != 0
        self.assertTrue(grep_result, "Output file {0} contains the word 'FATAL'...".format(outfile))

        # Test for expected output
        for i in range(ranks):
            if program == "hello":
                self.file_contains(f'{program_output}_{i}', get_hello_string(i, ranks, tracerank, threads))
            else:
                self.file_contains(f'{program_output}_{i}', get_reduce_string(i, ranks, size))

        # Test to make sure that each core did some work and sent something to its L1
        for i in range(threads):
            self.assert_nonzero_stat(statfile, f"core.read_requests.{i}")
            self.assert_nonzero_stat(statfile, f"cache_{i}.CacheMisses")

#######################

    def _setup_ariel_test_files(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setup_ariel_test_files() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()


        # Set the paths to the various directories
        self.ArielElementDir = os.path.abspath("{0}/../".format(test_path))
        self.ArielElementTestMPIDir = "{0}/tests/testMPI".format(self.ArielElementDir)
        self.ArielElementAPIDir = "{0}/api".format(self.ArielElementDir)

        # In case the user has not put the lib directory on their LD_LIBRARY_PATH
        os.environ["ARIELAPI"] = self.ArielElementAPIDir

        # Build the test mpi programs
        cmd = "make"
        rtn1 = OSCommand(cmd, set_cwd=self.ArielElementTestMPIDir).run()
        log_debug("Ariel tests/testMPI `make` result = {1}; output =\n{2}".format(self.ArielElementTestMPIDir, rtn1.result(), rtn1.output()))

        # Check that everything compiled OK
        self.assertTrue(rtn1.result() == 0, "MPI test binaries failed to compile")

