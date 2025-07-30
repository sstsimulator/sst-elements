# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
from sst_unittest_parameterized import parameterized
import subprocess
import re

module_init = 0
module_sema = threading.Semaphore()
golem_test_matrix = []

MakeTests = False
#MakeTests = True
updateFiles = False
#updateFiles = True

################################################################################
# NOTES:
# This is a parameterized test, we setup the golem test matrix ahead of time and
# then use the sst_unittest_parameterized module to load the list of testcases.
#
################################################################################

def check_for_crosssim_library():
    try:
        import simulator
        return True
    except ImportError:
        return False

def check_for_crosssim_components():
    binpath = sstsimulator_conf_get_value("SSTCore", "bindir", str) + "/sst-info"
    try:
        result = subprocess.run(
            [binpath, "-l", "golem"],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
    except subprocess.CalledProcessError as e:
        print("Error running sst-info:", e)
        return []

    output = result.stdout
    names = []
    pattern = re.compile(r"^SubComponent\s+\d+:\s+(.*)$")
    for line in output.splitlines():
        match = pattern.match(line.strip())
        if match:
            names.append(match.group(1).strip())
    return 'CrossSimIntArray' in names and 'CrossSimFloatArray' in names

def build_golem_test_matrix():
    global golem_test_matrix
    golem_test_matrix = []
    testlist = []

    arch_list = ["riscv64"]
    tests = {"single_array": 1, "multi_array": 2}

    if check_for_crosssim_components() and check_for_crosssim_library():
        location="small/crosssim_float_array"
        array_type = "crosssim_float"
        for test, num_arrays in tests.items():
            for arch in arch_list:
                testlist.append(["basic_golem.py", location, test,arch, 1, 1, array_type, num_arrays, "", 300])

        location="small/crosssim_int_array"
        array_type = "crosssim_int"
        for test, num_arrays in tests.items():
            for arch in arch_list:
                testlist.append(["basic_golem.py", location, test,arch, 1, 1, array_type, num_arrays, "", 300])

    location="small/mvm_float_array"
    array_type = "mvm_float"
    for test, num_arrays in tests.items():
        for arch in arch_list:
            testlist.append(["basic_golem.py", location, test,arch, 1, 1, array_type, num_arrays, "", 300])

    location="small/mvm_int_array"
    array_type = "mvm_int"
    for test, num_arrays in tests.items():
        for arch in arch_list:
            testlist.append(["basic_golem.py", location, test,arch, 1, 1, array_type, num_arrays, "", 300])


    # Process each line and crack up into an index, hash, options and sdl file
    for testnum, test_info in enumerate(testlist):
        # Make testnum start at 1
        testnum = testnum + 1
        sdlfile = test_info[0]
        elftestdir = test_info[1]
        elffile = test_info[2]
        isa = test_info[3]
        numCores = test_info[4]
        numHwThreads = test_info[5]
        arrayType = test_info[6]
        numArrays = test_info[7]
        goldfiledir = test_info[8]
        timeout_sec = test_info[9]
        testname = "{0}_{1}_{2}_{3}".format(elftestdir.replace("/", "_"), elffile,isa,goldfiledir)

        # Build the test_data structure
        test_data = (testnum, testname, sdlfile, elftestdir, elffile, isa, numCores, numHwThreads, arrayType, numArrays,  goldfiledir, timeout_sec )
        golem_test_matrix.append(test_data)

################################################################################

# At startup, build the test matrix
build_golem_test_matrix()

def gen_custom_name(testcase_func, param_num, param):
# Full TestCaseName
#    testcasename = "{0}_{1}".format(testcase_func.__name__,
#        parameterized.to_safe_name("_".join(str(x) for x in param.args)))
# Abbreviated TestCaseName
    testcasename = "{0}_{1:03}_{2}".format(testcase_func.__name__,
        int(parameterized.to_safe_name(str(param.args[0]))),
        parameterized.to_safe_name(str(param.args[1])))
    return testcasename


class testcase_golem(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test
        self._setupTests()

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    @parameterized.expand(golem_test_matrix, name_func=gen_custom_name)
    def test_golem_short_tests(self, testnum, testname, sdlfile, elftestdir, elffile, isa, numCores, numHwThreads, arrayType, numArrays, goldfiledir, timeout_sec):
        self._checkSkipConditions( isa )

        if MakeTests:
            self.makeTest( testname, isa, elftestdir, elffile )
        log_debug("Running Golem test #{0} ({1}): elffile={4} in dir {3}, isa {5}; using sdl={2}".format(testnum, testname, sdlfile, elftestdir, elffile, isa, timeout_sec))
        self.golem_test_template(testnum, testname, sdlfile, elftestdir, elffile, isa, numCores, numHwThreads, arrayType, numArrays, goldfiledir, timeout_sec )

#####

    def golem_test_template(self, testnum, testname, sdlfile, elftestdir, elffile, isa, numCores, numHwThreads, arrayType, numArrays, goldfiledir, testtimeout=120):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = "{0}/golem_tests/{1}/{2}/{3}/{4}".format(self.get_test_output_run_dir(), elftestdir,elffile,isa,goldfiledir)
        tmpdir = self.get_test_output_tmp_dir()
        os.makedirs(outdir)

        # Set the various file paths
        testDataFileName="test_golem_{0}".format(testname)
        sdlfile = "{0}/{1}".format(test_path, sdlfile)
        sst_outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        sst_errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        if len(goldfiledir):
            goldfiledir = goldfiledir + "/"

        ref_sst_outfile = "{0}/{1}/{2}/{3}/{4}sst.stdout.gold".format(test_path, elftestdir, elffile, isa, goldfiledir)
        ref_sst_errfile = "{0}/{1}/{2}/{3}/{4}sst.stderr.gold".format(test_path, elftestdir, elffile, isa, goldfiledir)
        ref_os_outfile = "{0}/{1}/{2}/{3}/{4}golem.stdout.gold".format(test_path, elftestdir, elffile, isa, goldfiledir)
        ref_os_errfile = "{0}/{1}/{2}/{3}/{4}golem.stderr.gold".format(test_path, elftestdir, elffile, isa, goldfiledir)
        # 100 is the pid
        os_outfile = "{0}/stdout-100".format(outdir)
        os_errfile = "{0}/stderr-100".format(outdir)

        # Set the Vanadis EXE path
        testfilepath = "{0}/{1}/{2}/{3}/{2}".format(test_path, elftestdir, elffile, isa )
        os.environ['VANADIS_EXE'] = testfilepath
        os.environ['VANADIS_ISA'] = "RISCV64"

        os.environ['VANADIS_NUM_CORES'] = str(numCores)
        os.environ['VANADIS_NUM_HW_THREADS'] = str(numHwThreads)

        array_types = ["crosssim_float", "crosssim_int", "mvm_float", "mvm_int"]
        arraytype_exists = arrayType in array_types
        self.assertTrue(arraytype_exists, "Array Type {} is not one of the following: {}".format(arrayType, array_types))

        if arrayType == "crosssim_float":
            roccType = "golem.RoCCAnalogFloat"
            arrayType = "golem.CrossSimFloatArray"

        elif arrayType == "crosssim_int":
            roccType = "golem.RoCCAnalogInt"
            arrayType = "golem.CrossSimIntArray"

        elif arrayType == "mvm_float":
            roccType = "golem.RoCCAnalogFloat"
            arrayType = "golem.MVMFloatArray"

        elif arrayType == "mvm_int":
            roccType = "golem.RoCCAnalogInt"
            arrayType = "golem.MVMIntArray"


        os.environ["GOLEM_ROCC_TYPE"] = roccType
        os.environ["GOLEM_ARRAY_TYPE"] = arrayType
        os.environ["GOLEM_NUM_ARRAYS"] = str(numArrays)

        testfile_exists = os.path.exists(testfilepath) and os.path.isfile(testfilepath)
        self.assertTrue(testfile_exists, "Golem test {0} does not exist".format(testfilepath))

        oscmd = self.run_sst(sdlfile, sst_outfile, sst_errfile, mpi_out_files=mpioutfiles, set_cwd=outdir, timeout_sec=testtimeout)

        # Perform the tests
        # Verify that the errfile from SST is empty
        if os_test_file(sst_errfile, "-s"):
            log_testing_note("golem test {0} has a Non-Empty Error File {1}".format(testDataFileName, sst_errfile))

        # Verify that a stdout-* and stderr-* files were generated by the Golem run
        os_outfileexists = os.path.exists(os_outfile) and os.path.isfile(os_outfile)
        os_errfileexists = os.path.exists(os_errfile) and os.path.isfile(os_errfile)
        self.assertTrue(os_outfileexists, "Golem test outfile-os not found in directory {0}".format(outdir))
        self.assertTrue(os_errfileexists, "Golem test errfile-os not found in directory {0}".format(outdir))

        if ( os.path.exists( ref_sst_outfile ) ):
            cmp_result = testing_compare_filtered_diff(testname, sst_outfile, ref_sst_outfile ,filters=[StartsWithFilter(" v0.instructions_issued.1")])
            if (cmp_result == False):
                diffdata = testing_get_diff_data(testname)
                log_failure(oscmd)
                log_failure(diffdata)

                if updateFiles:
                    print("Updating gold file ",sst_outfile, "->" ,ref_sst_outfile)
                    subprocess.call( [ "cp", sst_outfile, ref_sst_outfile ] )

            self.assertTrue(cmp_result, "Golem output file {0} does not match reference output file {1}".format(sst_outfile, ref_sst_outfile))
        else:
            log_testing_note("Golem test {0} SST gold file does not exist, did not compare".format(testDataFileName))

        cmp_result = testing_compare_diff(testname, os_outfile, ref_os_outfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)

            if updateFiles:
                print("Updating sst file ",os_outfile, "->" ,ref_os_outfile)
                subprocess.call( [ "cp", os_outfile, ref_os_outfile ] )

        self.assertTrue(cmp_result, "Golem os output file {0} does not match reference output file {1}".format(os_outfile, ref_os_outfile))

        cmp_result = testing_compare_diff(testname, os_errfile, ref_os_errfile)
        if (cmp_result == False):
            diffdata = testing_get_diff_data(testname)
            log_failure(oscmd)
            log_failure(diffdata)

            if updateFiles:
                print("Updating sst file ",os_errfile, "->" ,ref_os_errfile)
                subprocess.call( [ "cp", os_errfile, ref_os_errfile ] )

        self.assertTrue(cmp_result, "Golem os error file {0} does not match reference error file {1}".format(os_outfile, ref_os_outfile))

        # DEVELOPER NOTE: In the future, we may want to compare the SST output (statisics) vs some reference file


###############################################

    def _checkSkipConditions(self,isa):
        # Check to see if the musl compiler is missing
        if MakeTests:
            if self._is_musl_compiler_available(isa) == False:
                self.skipTest("Golem Skipping Test - musl compiler not available")

        if testing_check_get_num_ranks() > 1:
            self.skipTest("Golem Skipping Test - ranks > 1 not supported")

        if testing_check_get_num_threads() > 1:
            self.skipTest("Golem Skipping Test - threads > 1 not supported")

###
    def _is_musl_compiler_available(self,isa):

        # Now build the array application
        cmd = "which " + isa + "-linux-musl-gcc"
        rtn = OSCommand(cmd).run()
        log_debug("Golem detecting musl compiler [mipsel-linux-musl-gcc] - result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        return rtn.result() == 0

###

    def makeTest(self,testname,isa,elftestdir, elffile):

        test_path = self.get_testsuite_dir()

        sourcedirpath = "{0}/{1}/{2}".format( test_path, elftestdir, elffile )
        makefilepath = "{0}/Makefile".format(sourcedirpath)

        cmd = "make ARCH=" + isa
        rtn = OSCommand(cmd, set_cwd=sourcedirpath).run()
        log_debug("Golem tests source - Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "{0} failed to build properly".format(makefilepath))

    def _setupTests(self):
        return
