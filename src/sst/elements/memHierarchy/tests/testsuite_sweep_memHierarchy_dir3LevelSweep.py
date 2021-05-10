# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
from sst_unittest_parameterized import parameterized

import os
import subprocess

dirpath = os.path.dirname(sys.modules[__name__].__file__)

module_init = 0
module_sema = threading.Semaphore()
sweep_test_matrix = []

################################################################################
# NOTES:
# This is a parameterized test, we setup the sweep test matrix ahead of time and
# then use the sst_unittest_parameterized module to load the list of testcases.
#
# There are a lot of environment options that can be set by exporting a env variable
# that will affect testing operations:
# SST_SWEEP_MSIMESI_OPT = MSI | MESI
# SST_SWEEP_OPENMP = ompatomic | ompapi | ompcritical | ompdynamic | ompreduce | omptriangle | ompbarrier
# SST_SWEEP_dir3levelsweep_LIST=first-last  -- Sweep Tests to be run
#
################################################################################

def build_sweep_test_matrix():
    global sweep_test_matrix
    sweep_test_matrix = []
    testnum = 1

    # OMP App names
    omp_app_list = ["ompatomic", "ompapi", "ompcritical", "ompdynamic", "ompreduce", "omptriangle", "ompbarrier"]

    # Get the OpenMP application to run is defined in env (ompdynamic is default)
    env_var = 'SST_SWEEP_OPENMP'
    try:
        omp_app = os.environ[env_var]
        if not omp_app in omp_app_list:
            log_error ("Env Var {0} = {1}; is not valid; valid options = {2}".format(env_var, omp_app, omp_app_list))
            omp_app = "ompdynamic"
    except KeyError:
        omp_app = "ompdynamic"

    # L1 Cache Options
    L1_cache_size_list = ["8KB", "32KB"]
    L1_replacement_policy_list = ["random", "mru"]
    L1_associativity_list = ["2", "4"]

    # L2 Cache Options
    L2_cache_size_list = ["32KB", "128KB"]
    L2_replacement_policy_list = ["random", "mru"]
    L2_associativity_list = ["8", "16"]
    L2_mshr_size_list = ["8", "64"]

    # L3 Cache Options
    L3_cache_size_list = ["32KB", "128KB"]
    L3_replacement_policy_list = ["random", "mru"]
    L3_associativity_list = ["8", "16"]
    L3_mshr_size_list = ["8", "64"]

    # Test type options
    test_type_list = ["MSI", "MESI"]

    # See if an Test Type has been defined in the env
    env_var = 'SST_SWEEP_MSIMESI_OPT'
    try:
        forced_test_type = os.environ[env_var]
        if not forced_test_type in test_type_list:
            log_error ("Env Var {0} = {1}; is not valid; valid options = {2}".format(env_var, forced_test_type, test_type_list))
            forced_test_type = None
    except KeyError:
        forced_test_type = None

    # Now loop through the options and build the Test Matrix
    log_debug("*** BUILDING THE TEST MATRIX")
    for L1_size in L1_cache_size_list:
        if L1_size == "8 KB":
            L1_repl = "mru"
            L1_asso = "2"
        else:
            L1_repl = "random"
            L1_asso = "4"
        for L2_size in L2_cache_size_list:
            for L3_size in L3_cache_size_list:
                for L2_repl in L2_replacement_policy_list:
                    for L3_repl in L3_replacement_policy_list:
                        for L2_asso in L2_associativity_list:
                            for L3_asso in L3_associativity_list:
                                for L2_mshr in L2_mshr_size_list:
                                    for L3_mshr in L3_mshr_size_list:
                                        if forced_test_type == None:
                                            for test_type in test_type_list:
                                                _add_data_to_test_matrix(testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type)
                                                testnum += 1
                                        else:
                                            test_type = forced_test_type
                                            _add_data_to_test_matrix(testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type)
                                            testnum += 1

def _add_data_to_test_matrix(testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type):
    global sweep_test_matrix

#    log_debug("sweep Test Matrix:TestNum={0:04}; App={13}; L1:size={1}; repl={2}; asso={3}; L2:size={4}; repl={5}; asso={6}; mshr={7}; L3:size={8}; repl={9}; asso={10}; mshr={11}; test_type={12}".format(testnum, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type, omp_app))
    test_data = (testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type)

    # FOR DEBUG - disable tests above a certain number
    #if testnum > 10:
    #    return

    sweep_test_matrix.append(test_data)

################################################################################

# At startup, build the test matrix
build_sweep_test_matrix()

def gen_custom_name(testcase_func, param_num, param):
# Full TestCaseName
#    testcasename = "{0}_{1:03}".format(testcase_func.__name__,
#        parameterized.to_safe_name("_".join(str(x) for x in param.args)))
# Abbreviated TestCaseName
    testcasename = "{0}_{1:04}_{2}_L1:{3}-{4}-{5}_L2:{6}-{7}-{8}-{9}_L3:{10}-{11}-{12}-{13}_Type:{14}".format(
        testcase_func.__name__,                                   # Test Name
        int(parameterized.to_safe_name(str(param.args[0]))),      # Test Num
        parameterized.to_safe_name(str(param.args[1])),           # OMP App

        parameterized.to_safe_name(str(param.args[2])),           # L1 Cache
        parameterized.to_safe_name(str(param.args[3])),
        parameterized.to_safe_name(str(param.args[4])),

        parameterized.to_safe_name(str(param.args[5])),           # L2 Cache
        parameterized.to_safe_name(str(param.args[6])),
        parameterized.to_safe_name(str(param.args[7])),
        parameterized.to_safe_name(str(param.args[8])),

        parameterized.to_safe_name(str(param.args[9])),           # L3 Cache
        parameterized.to_safe_name(str(param.args[10])),
        parameterized.to_safe_name(str(param.args[11])),
        parameterized.to_safe_name(str(param.args[12])),

        parameterized.to_safe_name(str(param.args[13]))           # Test Type
        )
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
            pass
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_memH_sweep_dir3levelsweep(SSTTestCase):

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

####

    @parameterized.expand(sweep_test_matrix, name_func=gen_custom_name)
    def test_memH_sweep_dir3levelsweep(self, testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type):
        self._checkSkipConditions(testnum)

        log_debug("Running memH sweep dir3levelsweep #{0:04}; App={13}; L1:size={1}; repl={2}; asso={3}; L2:size={4}; repl={5}; asso={6}; mshr={7}; L3:size={8}; repl={9}; asso={10}; mshr={11}; test_type={12}".format(testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type))
        self.memH_sweep_test_template(testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type)

####

    def memH_sweep_test_template(self, testnum, omp_app, L1_size, L1_repl, L1_asso, L2_size, L2_repl, L2_asso, L2_mshr, L3_size, L3_repl, L3_asso, L3_mshr, test_type):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_memH_sweep_dir3LevelSweep_{0}_case".format(omp_app)

        reffile = "{0}/refFiles/test_OMP_{1}.out".format(test_path, omp_app)
        outfile = "{0}/{1}_{2:04}.out".format(outdir, testDataFileName, testnum)
        errfile = "{0}/{1}_{2:04}.err".format(outdir, testDataFileName, testnum)
        mpioutfiles = "{0}/{1}_{2:04}.testfile".format(outdir, testDataFileName, testnum)
        sdlfile = "{0}/openMP/test-distributed-caches.py".format(test_path)
        testtimeout = 120

        # Set the environment path to the omp application used by the SDL file
        # and verify it exists
        ompapppath = "{0}/openMP/{1}/{1}".format(test_path, omp_app)
        self.assertTrue(os.path.isfile(ompapppath), "Sweep dir3LevelSweep Test; ompfile {0} - not found".format(ompapppath))
        os.environ["OMP_EXE"] = ompapppath

        # Set the model options for the test (This is what we sweep)
        otherargs = '--model-options="--L1cachesz={0} --L2cachesz={1} --L3cachesz={2} --L1Replacp={3} --L2Replacp={4} --L3Replacp={5} --L1assoc={6} --L2assoc={7} --L3assoc={8} --L2MSHR={9} --L3MSHR={10} --MSIMESI={11}" '.format(
                    L1_size,
                    L2_size,
                    L3_size,
                    L1_repl,
                    L2_repl,
                    L3_repl,
                    L1_asso,
                    L2_asso,
                    L3_asso,
                    L2_mshr,
                    L3_mshr,
                    test_type)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, mpi_out_files=mpioutfiles, timeout_sec=testtimeout)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Dig through the output file looking for "Simulation is complete"
        outfoundline = ""
        grepstr = 'Simulation is complete'
        with open(outfile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    outfoundline = line

        outtestresult = outfoundline != ""
        self.assertTrue(outtestresult, "Sweep dir3LevelSweep Test {0} - Cannot find string \"{1}\" in output file {2}".format(testnum, grepstr, outfile))

        # Now dig throught the reference file line by line and then grep the counts
        # of each line in both the reffile and outfile to ensure they match
        outcount = 0
        refcount = 0
        with open(reffile, 'r') as f:
            for line in f.readlines():
                testline = line.strip()
                if testline != "":
                    outcount = 0
                    refcount = 0
                    # Grep the ref file for the count of this line occuring
                    cmd = 'grep -c "{0}" {1}'.format(testline, reffile)
                    rtn = OSCommand(cmd).run()
                    self.assertEquals(rtn.result(), 0, "Sweep dir3LevelSweep Test failed running cmdline {0} - grepping reffile {1}".format(cmd, reffile))
                    refcount = int(rtn.output())

                    # Grep the out file for the count of this line occuring
                    cmd = 'grep -c "{0}" {1}'.format(testline, outfile)
                    rtn = OSCommand(cmd).run()
                    if rtn.result() == 0:
                        outcount = int(rtn.output())
                    else:
                        log_failure("FAILURE: Sweep dir3LevelSweep Test failed running cmdline {0} - grepping outfile {1}".format(cmd, outfile))

                    log_debug("Testline='{0}'; refcount={1}; outcount={2}".format(testline, refcount, outcount))

                    # Compare the count
                    self.assertEquals(outcount, refcount, "Sweep dir3LevelSweep testing line '{0}': outfile count = {1} does not match reffile count = {2}".format(testline, outcount, refcount))

###############################################

    def _checkSkipConditions(self, testindex):
        # Check to see if PIN is loaded
        pin_loaded = testing_is_PIN_loaded()
        if not pin_loaded:
            self.skipTest("Skipping Test #{0} - sweep_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.".format(testindex))

        # Check to see if Env Var SST_SWEEP_dir3levelsweep_LIST is set limiting which tests to run
        #   An inclusive sub-list may be specified as "first-last"  (e.g. 7-10)
        env_var = 'SST_SWEEP_dir3levelsweep_LIST'
        try:
            testlist = os.environ[env_var]
            log_debug("SST_SWEEP_dir3levelsweep_LIST = {0}; type = {1}".format(testlist, type(testlist)))
            index = testlist.find('-')
            if index > 0 and len(testlist) >= 3:
                startnumstr = int(testlist[0:index])
                stopnumstr = int(testlist[index+1:])
                try:
                    startnum = int(startnumstr)
                    stopnum = int(stopnumstr)
                except ValueError:
                    log_debug("Cannot decode start/stop index strings: startstr = {0}; stopstr = {1}".format(startnumstr, stopnumstr))
                    return
                log_debug("Current Test Index = {0}; Skip Number: startnum = {1}; stopnum = {2}".format(testindex, startnum, stopnum))
                if testindex < startnum or testindex > stopnum:
                    self.skipTest("Skipping Test #{0} - Per {1} = {2}".format(testindex, env_var, testlist))
            else:
                log_debug("Env Var {0} - not formatted correctly = {1}".format(env_var, testlist))
                return
        except KeyError:
            return


###############################################################################

