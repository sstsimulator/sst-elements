# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
from sst_unittest_parameterized import parameterized

module_init = 0
module_sema = threading.Semaphore()
rdmaNic_test_matrix = []

################################################################################
# NOTES:
# This is a parameterized test, we setup the rdmaNic test matrix ahead of time and
# then use the sst_unittest_parameterized module to load the list of testcases.
#
################################################################################

def build_rdmaNic_test_matrix():
    global rdmaNic_test_matrix
    rdmaNic_test_matrix = []
    testlist = []

    # Add the SDL file, test dir compiled elf file, and test run timeout to create the testlist
    testlist.append(["runVanadis.py", "app/rdma", "msg", 120])

    # Process each line and crack up into an index, hash, options and sdl file
    for testnum, test_info in enumerate(testlist):
        # Make testnum start at 1
        testnum = testnum + 1
        sdlfile = test_info[0]
        elftestdir = test_info[1]
        elffile = test_info[2]
        timeout_sec = test_info[3]
        testname = "{0}_{1}".format(elftestdir.replace("/", "_"), elffile)

        # Build the test_data structure
        test_data = (testnum, testname, sdlfile, elftestdir, elffile, timeout_sec)
        rdmaNic_test_matrix.append(test_data)

################################################################################

# At startup, build the RDMA test matrix
build_rdmaNic_test_matrix()

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
            class_inst._setupRdmaNicTestFiles()
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_rdmaNic(SSTTestCase):

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

    @parameterized.expand(rdmaNic_test_matrix, name_func=gen_custom_name)
    def test_rdmaNic_short_tests(self, testnum, testname, sdlfile, elftestdir, elffile, timeout_sec):
        self._checkSkipConditions()

        log_debug("Running RdmaNic test #{0} ({1}): elffile={4} in dir {3}; using sdl={2}".format(testnum, testname, sdlfile, elftestdir, elffile, timeout_sec))
        self.rdmaNic_test_template(testnum, testname, sdlfile, elftestdir, elffile, timeout_sec)

#####

    def rdmaNic_test_template(self, testnum, testname, sdlfile, elftestdir, elffile, testtimeout=120):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = "{0}/rdmaNic_tests/{1}/{2}".format(self.get_test_output_run_dir(), elftestdir,elffile)
        tmpdir = self.get_test_output_tmp_dir()
        os.makedirs(outdir)

        # Set the various file paths
        testDataFileName="test_rdmaNic_{0}".format(testname)
        sdlfile = "{0}/{1}".format(test_path, sdlfile)

        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        ref_outfile = "{0}/{1}/{2}.out.gold".format(test_path, elftestdir, elffile)
        ref_errfile = "{0}/{1}/{2}.err.gold".format(test_path, elftestdir, elffile)

        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        node0_os_outfile = "{0}/stdout-node0.cpu0.os".format(test_path)
        node0_os_errfile = "{0}/stderr-node0.cpu0.os".format(test_path)
        ref_node0_os_outfile = "{0}/{1}/{2}.stdout-node0.cpu0.os.gold".format(test_path, elftestdir, elffile)
        ref_node0_os_errfile = "{0}/{1}/{2}.stderr-node0.cpu0.os.gold".format(test_path, elftestdir, elffile)

        node1_os_outfile = "{0}/stdout-node1.cpu0.os".format(test_path)
        node1_os_errfile = "{0}/stderr-node1.cpu0.os".format(test_path)
        ref_node1_os_outfile = "{0}/{1}/{2}.stdout-node1.cpu0.os.gold".format(test_path, elftestdir, elffile)
        ref_node1_os_errfile = "{0}/{1}/{2}.stderr-node1.cpu0.os.gold".format(test_path, elftestdir, elffile)

        # Set the RdmaNic EXE path
        testfilepath = "{0}/{1}/{2}".format(test_path, elftestdir, elffile)
        os.environ['VANADIS_EXE'] = testfilepath

        oscmd = self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles, set_cwd=test_path, timeout_sec=testtimeout)

        # Perform the tests
        # Verify that the errfile from SST is empty
        if os_test_file(errfile, "-s"):
            log_testing_note("rdmaNic test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        # Verify that a stdout-os and stderr-os files were generated by the RdmaNic run
        file_exists = os.path.exists(node0_os_outfile) and os.path.isfile(node0_os_outfile)
        self.assertTrue(file_exists, "RdmaNic test {0} not found in directory {1}".format(node0_os_outfile,outdir))
        file_exists = os.path.exists(node0_os_errfile) and os.path.isfile(node0_os_errfile)
        self.assertTrue(file_exists, "RdmaNic test {0} not found in directory {1}".format(node0_os_errfile,outdir))

        file_exists = os.path.exists(node1_os_outfile) and os.path.isfile(node1_os_outfile)
        self.assertTrue(file_exists, "RdmaNic test {0} not found in directory {1}".format(node1_os_outfile,outdir))
        file_exists = os.path.exists(node1_os_errfile) and os.path.isfile(node1_os_errfile)
        self.assertTrue(file_exists, "RdmaNic test {0} not found in directory {1}".format(node1_os_errfile,outdir))

        cmp_result = testing_compare_diff(testname, outfile, ref_outfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "RdmaNic output file {0} does not match reference output file {1}".format(outfile, ref_outfile))

        cmp_result = testing_compare_diff(testname, errfile, ref_errfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "RdmaNic error file {0} does not match reference error file {1}".format(errfile, ref_errfile))

        cmp_result = testing_compare_diff(testname, node0_os_outfile, ref_node0_os_outfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "RdmaNic os output file {0} does not match reference output file {1}".format(node0_os_outfile, ref_node0_os_outfile))

        cmp_result = testing_compare_diff(testname, node0_os_errfile, ref_node0_os_errfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "RdmaNic os error file {0} does not match reference error file {1}".format(node0_os_errfile, ref_node0_os_errfile))

        cmp_result = testing_compare_diff(testname, node1_os_outfile, ref_node1_os_outfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "RdmaNic os output file {0} does not match reference output file {1}".format(node1_os_outfile, ref_node1_os_outfile))

        cmp_result = testing_compare_diff(testname, node1_os_errfile, ref_node1_os_errfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)
        self.assertTrue(cmp_result, "RdmaNic os error file {0} does not match reference error file {1}".format(node1_os_errfile, ref_node1_os_errfile))

        # DEVELOPER NOTE: In the future, we may want to compare the SST output (statisics) vs some reference file


###############################################

    def _checkSkipConditions(self):
        # Check to see if the musl compiler is missing
        if self._is_musl_compiler_available() == False:
            self.skipTest("RdmaNic Skipping Test - musl compiler not available")

        if testing_check_get_num_ranks() > 1:
            self.skipTest("RdmaNic Skipping Test - ranks > 1 not supported")

        if testing_check_get_num_threads() > 1:
            self.skipTest("RdmaNic Skipping Test - threads > 1 not supported")

###
    def _is_musl_compiler_available(self):

        # Now build the array application
        cmd = "which mipsel-linux-musl-gcc"
        rtn = OSCommand(cmd).run()
        log_debug("RdmaNic detecting musl compiler [mipsel-linux-musl-gcc] - result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        return rtn.result() == 0

###

    def _setupRdmaNicTestFiles(self):
        log_debug("_setupRdmaNicTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Detect if the musl compiler is available
        if self._is_musl_compiler_available() == False:
            log_debug("NOTICE: RdmaNic Testing - musl compiler not found")
            return

        # Walk the directory of source files and try to compile each of them
        mainsourcedir = "{0}/app".format(test_path)

        # For each subdir under the main source dir call the makefile
        for f in os.listdir(mainsourcedir):
            sourcedirpath = "{0}/{1}".format(mainsourcedir, f)
            makefilepath = "{0}/Makefile".format(sourcedirpath, f)
            if os.path.isdir(sourcedirpath) and os.path.isfile(makefilepath):
                log_debug("RdmaNic calling make on makefile {0}".format(makefilepath))

                # Now build the array application
                cmd = "make"
                rtn = OSCommand(cmd, set_cwd=sourcedirpath).run()
                log_debug("RdmaNic tests source - Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
                self.assertTrue(rtn.result() == 0, "{0} failed to build properly".format(makefilepath))
