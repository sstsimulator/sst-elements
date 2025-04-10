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

def get_hello_string(rank, ranks, tracerank, threads, frontend):
    if rank == tracerank and frontend == "pin":
        return [f"Hello from rank {rank} of {ranks}, thread {i}! (Launched by pin)\n" for i in range(threads)]
    else:
        return [f"Hello from rank {rank} of {ranks}, thread {i}!\n" for i in range(threads)]
################################################################################
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
    epax_loaded = is_EPAX_loaded()
    pebil_loaded = is_PEBIL_loaded()
    epa_loaded = epax_loaded or pebil_loaded
    pin_error_msg = "Ariel: PIN tests require PIN, but Env Var 'INTEL_PIN_DIRECTORY' is not found or path does not exist."
    epa_error_msg = "Ariel: EPA tests require PEBIL or EPAX, but Env Var 'PEBIL_ROOT' or 'EPAX_ROOT' is not found or path does not exist."

    multi_rank = testing_check_get_num_ranks() > 1
    multi_rank_error_msg = "Ariel: Ariel MPI tests are not compatible with multi-rank sst runs."

    using_osx = host_os_get_distribution_type() == OS_DIST_OSX
    osx_error_msg = "Ariel: OpenMP is not supported on macOS"

    # TODO: This is hacky. What is the correct way to get the test script location?
    testsuite_dir = os.path.dirname(__file__)
    mpilauncher_exists = os.path.isfile(testsuite_dir + '/../mpi/mpilauncher')
    mpi_error_msg = f"Ariel: The mpilauncher executable was not found"

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_01_pin(self):
        self.ariel_Template(threads=1, ranks=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_01_epa(self):
        self.ariel_Template(threads=1, ranks=1, frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_02_pin(self):
        self.ariel_Template(threads=1, ranks=2)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_02_epa(self):
        self.ariel_Template(threads=1, ranks=2, frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_03_pin(self):
        self.ariel_Template(threads=2, ranks=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_03_epa(self):
        self.ariel_Template(threads=2, ranks=1, frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_04_pin(self):
        self.ariel_Template(threads=1, ranks=2, tracerank=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_hello_04_epa(self):
        self.ariel_Template(threads=1, ranks=2, tracerank=1, frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_05_pin(self):
        self.ariel_Template(threads=2, ranks=3, tracerank=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_05_epa(self):
        self.ariel_Template(threads=2, ranks=3, tracerank=1, frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_06_pin(self):
        self.ariel_Template(threads=2, ranks=2)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_hello_06_epa(self):
        self.ariel_Template(threads=2, ranks=2, frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_reduce_01_pin(self):
        self.ariel_Template(threads=1, ranks=1, program="reduce")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    def test_Ariel_mpi_reduce_01_epa(self):
        self.ariel_Template(threads=1, ranks=1, program="reduce", frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_reduce_02_pin(self):
        self.ariel_Template(threads=2, ranks=2, program="reduce")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_reduce_02_epa(self):
        self.ariel_Template(threads=2, ranks=2, program="reduce", frontend="epa")

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not pin_loaded, pin_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_reduce_03_pin(self):
        self.ariel_Template(threads=2, ranks=4, program="reduce", tracerank=1)

    @unittest.skipIf(not mpilauncher_exists, mpi_error_msg)
    @unittest.skipIf(not epa_loaded, epa_error_msg)
    @unittest.skipIf(multi_rank, multi_rank_error_msg)
    @unittest.skipIf(using_osx, osx_error_msg)
    def test_Ariel_mpi_reduce_03_epa(self):
        self.ariel_Template(threads=2, ranks=4, program="reduce", tracerank=1, frontend="epa")

    def ariel_Template(self, threads, ranks, program="hello", tracerank=0, testtimeout=60, size=8000, frontend="pin"):
        # Set the paths to the various directories
        testcase = inspect.stack()[1][3] # name the test after the calling function

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # For epa frontend, use instrumented application
        if frontend == "epa":
            program = program + ".arielinst"

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
        other_args = f'--model-options="{program} -o {program_output} -r {ranks} -t {threads} -f {frontend} -a {tracerank} -s {size}"'

        log_debug("testcase = {0}".format(testcase))
        log_debug("frontend = {0}".format(frontend))
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
        log_debug("program_output_files = " + str(program_output_files))

        # Look for the word "FATAL" in the sst output file
        cmd = 'grep "FATAL" {0} '.format(outfile)
        grep_result = os.system(cmd) != 0
        self.assertTrue(grep_result, "Output file {0} contains the word 'FATAL'...".format(outfile))

        # Test for expected output
        for i in range(ranks):
            if program.startswith("hello"):
                self.file_contains(f'{program_output}_{i}', get_hello_string(i, ranks, tracerank, threads, frontend))
            else:
                self.file_contains(f'{program_output}_{i}', get_reduce_string(i, ranks, size))

        # Test to make sure that each core did some work and sent something to its L1
        log_debug("Checking stat file = {0}".format(statfile))
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

        # Everything compiled OK. Check if we need to check for epa binaries
        epax_loaded = is_EPAX_loaded()
        pebil_loaded = is_PEBIL_loaded()
        epa_loaded = epax_loaded or pebil_loaded

        if not epa_loaded:
            return

        # If doing epa frontend tests, check if hello and reduce are
        # instrumented. If not, instrument them
        if (not os.path.exists(self.ArielElementTestMPIDir + "/hello.arielinst")):
            # Instrument with pebil
            if (pebil_loaded):
                cmd = "pebil --tool ArielIntercept --app hello --inp + --threaded"
                rtn2 = OSCommand(cmd, set_cwd=self.ArielElementTestMPIDir).run()
                log_debug("Ariel tests/testMPI pebil result = {0}; output =\n{1}".format(rtn2.result(), rtn2.output()))
                if rtn2.result() != 0:
                    log_debug("Ariel {0} pebil returned non-zero exit code. Error:\n{1}".format(self.ArielElementTestMPIDir, rtn2.error()))
            # Instrument with epax
            else:
                self.assertTrue(False, "epax commands not implemented yet")
            # Check that everything instrumented OK
            self.assertTrue(rtn2.result() == 0, "hello app failed to instrument.\nResult was {0}.\nStandard out was:\n{1}\nStandard err was:\n{2}".format(rtn2.result(), rtn2.output(), rtn2.error()))

        if (not os.path.exists(self.ArielElementTestMPIDir + "/reduce.arielinst")):
            # Instrument with pebil
            if (pebil_loaded):
                cmd = "pebil --tool ArielIntercept --app reduce --inp + --threaded"
                rtn3 = OSCommand(cmd, set_cwd=self.ArielElementTestMPIDir).run()
                log_debug("Ariel tests/testMPI pebil result = {0}; output =\n{1}".format(rtn3.result(), rtn3.output()))
                if rtn3.result() != 0:
                    log_debug("Ariel {0} pebil returned non-zero exit code. Error:\n{1}".format(self.ArielElementTestMPIDir, rtn3.error()))
            # Instrument with epax
            else:
                self.assertTrue(False, "epax commands not implemented yet")
            # Check that everything instrumented OK
            self.assertTrue(rtn3.result() == 0, "reduce app failed to instrument.\nResult was {0}.\nStandard out was:\n{1}\nStandard err was:\n{2}".format(rtn3.result(), rtn3.output(), rtn3.error()))
