# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
from sst_unittest_parameterized import parameterized

module_init = 0
module_sema = threading.Semaphore()
vanadis_test_matrix = []

################################################################################
# NOTES:
# This is a parameterized test, we setup the vanadis test matrix ahead of time and
# then use the sst_unittest_parameterized module to load the list of testcases.
#
################################################################################

def build_vanadis_test_matrix():
    global vanadis_test_matrix
    vanadis_test_matrix = []
    testlist = []

    # Add the SDL file, test dir compiled elf file, and test run timeout to create the testlist
    testlist.append(["basic_vanadis.py", "small/basic-io", "hello-world",     "mipsel", 120])
    #testlist.append(["basic_vanadis.py", "small/basic-io", "hello-world",     "riscv64", 120])
    testlist.append(["basic_vanadis.py", "small/basic-io", "hello-world-cpp", "mipsel", 120])
    testlist.append(["basic_vanadis.py", "small/basic-io", "printf-check",    "mipsel", 120])
    testlist.append(["basic_vanadis.py", "small/basic-io", "openat",          "mipsel", 120])
    testlist.append(["basic_vanadis.py", "small/basic-io", "unlink",          "mipsel", 120])
    testlist.append(["basic_vanadis.py", "small/basic-io", "unlinkat",        "mipsel", 120])
    testlist.append(["basic_vanadis.py", "small/basic-math", "sqrt-double",   "mipsel", 300])
    testlist.append(["basic_vanadis.py", "small/basic-math", "sqrt-float",    "mipsel", 300])
    testlist.append(["basic_vanadis.py", "small/basic-ops", "test-branch",    "mipsel", 300])
    testlist.append(["basic_vanadis.py", "small/basic-ops", "test-shift",     "mipsel",300])

    # Process each line and crack up into an index, hash, options and sdl file
    for testnum, test_info in enumerate(testlist):
        # Make testnum start at 1
        testnum = testnum + 1
        sdlfile = test_info[0]
        elftestdir = test_info[1]
        elffile = test_info[2]
        isa = test_info[3]
        timeout_sec = test_info[4]
        testname = "{0}_{1}_{2}".format(elftestdir.replace("/", "_"), elffile,isa)

        # Build the test_data structure
        test_data = (testnum, testname, sdlfile, elftestdir, elffile, isa, timeout_sec)
        vanadis_test_matrix.append(test_data)

################################################################################

# At startup, build the ESshmem test matrix
build_vanadis_test_matrix()

def gen_custom_name(testcase_func, param_num, param):
# Full TestCaseName
#    testcasename = "{0}_{1}".format(testcase_func.__name__,
#        parameterized.to_safe_name("_".join(str(x) for x in param.args)))
# Abbreviated TestCaseName
    testcasename = "{0}_{1:03}_{2}".format(testcase_func.__name__,
        int(parameterized.to_safe_name(str(param.args[0]))),
        parameterized.to_safe_name(str(param.args[1])))
    return testcasename

################################################################################
# Code to support a single instance module initialize, must be called setUp method

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        try:
            # Put your single instance Init Code Here
            class_inst._setupESshmemSmallTestFiles()
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_vanadis(SSTTestCase):

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

    @parameterized.expand(vanadis_test_matrix, name_func=gen_custom_name)
    def test_vanadis_short_tests(self, testnum, testname, sdlfile, elftestdir, elffile, isa, timeout_sec):
        self._checkSkipConditions()

        log_debug("Running Vanadis test #{0} ({1}): elffile={4} in dir {3}, isa {5}; using sdl={2}".format(testnum, testname, sdlfile, elftestdir, elffile, isa, timeout_sec))
        self.vanadis_test_template(testnum, testname, sdlfile, elftestdir, elffile, isa, timeout_sec)

#####

    def vanadis_test_template(self, testnum, testname, sdlfile, elftestdir, elffile, isa, testtimeout=120):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = "{0}/vanadis_tests/{1}/{2}/{3}".format(self.get_test_output_run_dir(), elftestdir,elffile,isa)
        tmpdir = self.get_test_output_tmp_dir()
        os.makedirs(outdir)

        # Set the various file paths
        testDataFileName="test_vanadis_{0}".format(testname)
        sdlfile = "{0}/{1}".format(test_path, sdlfile)
        sst_outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        sst_errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        ref_sst_outfile = "{0}/{1}/{2}/{3}/sst.stdout.gold".format(test_path, elftestdir, elffile, isa)
        ref_sst_errfile = "{0}/{1}/{2}/{3}/sst.stderr.gold".format(test_path, elftestdir, elffile, isa)
        ref_os_outfile = "{0}/{1}/{2}/{3}/vanadis.stdout.gold".format(test_path, elftestdir, elffile, isa)
        ref_os_errfile = "{0}/{1}/{2}/{3}/vanadis.stderr.gold".format(test_path, elftestdir, elffile, isa)
        os_outfile = "{0}/stdout-os".format(outdir)
        os_errfile = "{0}/stderr-os".format(outdir)

        # Set the Vanadis EXE path
        testfilepath = "{0}/{1}/{2}/{3}/{2}".format(test_path, elftestdir, elffile, isa )
        os.environ['VANADIS_EXE'] = testfilepath
        if isa == "mipsel":
            os.environ['VANADIS_ISA'] = "MIPS"
        else:
            os.environ['VANADIS_ISA'] = "RISCV64"

        testfile_exists = os.path.exists(testfilepath) and os.path.isfile(testfilepath)
        self.assertTrue(testfile_exists, "Vanadis test {0} does not exist".format(testfilepath))

        oscmd = self.run_sst(sdlfile, sst_outfile, sst_errfile, mpi_out_files=mpioutfiles, set_cwd=outdir, timeout_sec=testtimeout)

        # Perform the tests
        # Verify that the errfile from SST is empty
        if os_test_file(sst_errfile, "-s"):
            log_testing_note("vanadis test {0} has a Non-Empty Error File {1}".format(testDataFileName, sst_errfile))

        # Verify that a stdout-os and stderr-os files were generated by the Vanadis run
        os_outfileexists = os.path.exists(os_outfile) and os.path.isfile(os_outfile)
        os_errfileexists = os.path.exists(os_errfile) and os.path.isfile(os_errfile)
        self.assertTrue(os_outfileexists, "Vanadis test outfile-os not found in directory {0}".format(outdir))
        self.assertTrue(os_errfileexists, "Vanadis test errfile-os not found in directory {0}".format(outdir))

        #print( "diff", sst_outfile, ref_sst_outfile )
        #if ( os.path.exists( ref_sst_outfile ) ):
        #    cmp_result = testing_compare_filtered_diff(testname, sst_outfile, ref_sst_outfile ,filters=[StartsWithFilter(" v0.instructions_issued.1")])
        #    if (cmp_result == False):
        #        diffdata = testing_get_diff_data(testname)
        #        log_failure(oscmd)
        #        log_failure(diffdata)
        #    self.assertTrue(cmp_result, "Vanadis output file {0} does not match reference output file {1}".format(sst_outfile, ref_sst_outfile))
        #else:
        #    log_testing_note("vanadis test {0} SST gold file does not exist, did not compare".format(testDataFileName))

        cmp_result = testing_compare_diff(testname, os_outfile, ref_os_outfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Vanadis os output file {0} does not match reference output file {1}".format(os_outfile, ref_os_outfile))

        cmp_result = testing_compare_diff(testname, os_errfile, ref_os_errfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "Vanadis os error file {0} does not match reference error file {1}".format(os_outfile, ref_os_outfile))

        # DEVELOPER NOTE: In the future, we may want to compare the SST output (statisics) vs some reference file


###############################################

    def _checkSkipConditions(self):
        # Check to see if the musl compiler is missing
        if self._is_musl_compiler_available() == False:
            self.skipTest("Vanadis Skipping Test - musl compiler not available")

        if testing_check_get_num_ranks() > 1:
            self.skipTest("Vanadis Skipping Test - ranks > 1 not supported")

        if testing_check_get_num_threads() > 1:
            self.skipTest("Vanadis Skipping Test - threads > 1 not supported")

###
    def _is_musl_compiler_available(self):

        # Now build the array application
        cmd = "which mipsel-linux-musl-gcc"
        rtn = OSCommand(cmd).run()
        log_debug("Vanadis detecting musl compiler [mipsel-linux-musl-gcc] - result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        return rtn.result() == 0

###

    def _setupESshmemSmallTestFiles(self):
        log_debug("_setupVanadisSmallTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Detect if the musl compiler is available
        if self._is_musl_compiler_available() == False:
            log_debug("NOTICE: Vanadis Testing - musl compiler not found")
            return

        # Walk the directory of source files and try to compile each of them
        mainsourcedir = "{0}/small".format(test_path)

        # For each subdir under the main source dir call the makefile
        for f in os.listdir(mainsourcedir):
            sourcedirpath = "{0}/{1}".format(mainsourcedir, f)
            makefilepath = "{0}/Makefile".format(sourcedirpath, f)
            if os.path.isdir(sourcedirpath) and os.path.isfile(makefilepath):
                log_debug("Vanadis calling make on makefile {0}".format(makefilepath))

                # Now build the array application
                cmd = "make"
                print(  cmd, sourcedirpath )
                rtn = OSCommand(cmd, set_cwd=sourcedirpath).run()
                log_debug("Vanadis tests source - Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
                self.assertTrue(rtn.result() == 0, "{0} failed to build properly".format(makefilepath))


