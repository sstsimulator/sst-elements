# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
from sst_unittest_parameterized import parameterized

import hashlib
#import os

dirpath = os.path.dirname(sys.modules[__name__].__file__)

module_init = 0
module_sema = threading.Semaphore()
ESshmem_test_matrix = []

################################################################################
# NOTES:
# This is a parameterized test, we setup the ESshmem test matrix ahead of time and
# then use the sst_unittest_parameterized module to load the list of testcases.
#
################################################################################

def build_ESshmem_test_matrix():
    global ESshmem_test_matrix
    ESshmem_test_matrix = []
    testlist = []

    # Add all the tests to the matrix
    testlist_filepath = "{0}/ESshmem_List-of-Tests".format(dirpath)
    #log_debug("ESshmem: testlist_filepath = {0}".format(testlist_filepath))

    with open(testlist_filepath, 'r') as f:
            for line in f:
                testlist.append(line)

    # Process each line and crack up into an index, hash, options and sdl file
    for testnum, test_str in enumerate(testlist):
        # Make testnum start at 1
        testnum = testnum + 1

        # Get the Hash
        hash_object = hashlib.md5(test_str.encode('utf-8'))
        hash_str = hash_object.hexdigest()

        # Find the index to where the sdl file lives and then extract it
        index = test_str.rfind('"')
        if index == -1:
            log_debug("Error: Cannot find last \" in test string {0}".format(test_str))
            continue

        sdlfile = test_str[index+1:].replace("\n", "").lstrip()

        # Strip the sst from the front and sdl filename from back
        modelstr = test_str[0:index+1].replace("sst ", "")
        #log_debug("ESshmem Test Matrix:TestNum={0:03}; HASH={1}; sdlfile={2}; modelstr={3}".format(testnum, hash_str, sdlfile, modelstr))

        # Build the test_data structure
        test_data = (testnum, hash_str, modelstr, sdlfile)
        ESshmem_test_matrix.append(test_data)


################################################################################

# At startup, build the ESshmem test matrix
build_ESshmem_test_matrix()

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
            class_inst._setupESshmemTestFiles()
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_Ember_ESshmem(SSTTestCase):

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

    @parameterized.expand(ESshmem_test_matrix, name_func=gen_custom_name)
    def test_Ember_ESshmem(self, testnum, hash_str, modelstr, sdlfile):
        self._checkSkipConditions(testnum)

        log_debug("Running Ember ESshmem #{0} ({1}): model={2} using sdl={3}".format(testnum, hash_str, modelstr, sdlfile))
        self.Ember_ESshmem_test_template(testnum, hash_str, modelstr, sdlfile)

####

    def Ember_ESshmem_test_template(self, testnum, hash_str, modelstr, sdlfile):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.ember_ESshmem_Folder = "{0}/ember_ESshmem_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Set the various file paths
        testDataFileName="{0}".format("testESshmem_{0:03}".format(testnum))

        reffile = "{0}/refFiles/ESshmem_cumulative.out".format(test_path)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/{1}".format(test_path, sdlfile)
        testtimeout = 120

        otherargs = modelstr

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, set_cwd=self.ember_ESshmem_Folder, mpi_out_files=mpioutfiles, timeout_sec=testtimeout)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        if os_test_file(errfile, "-s"):
            log_testing_note("Ember test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        # Dig through the output file looking for "Simulation is complete"
        outfoundline = ""
        grepstr = 'Simulation is complete'
        with open(outfile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    outfoundline = line

        outtestresult = outfoundline != ""
        self.assertTrue(outtestresult, "Ember Test Test {0} - Cannot find string \"{1}\" in output file {2}".format(testnum, grepstr, outfile))

        reffoundline = ""
        grepstr = '{0} {1}'.format(hash_str, outfoundline)
        with open(reffile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    reffoundline = line

        reftestresult = reffoundline != ""
        self.assertTrue(reftestresult, "Ember Test Test {0} - Cannot find string \"{1}\" in reference file {2}".format(testnum, grepstr, reffile))

        log_debug("Ember Test Test {0} - PASSED\n--------".format(testnum))

###############################################

    def _checkSkipConditions(self, testindex):
        # Check to see if Env Var SST_TEST_ESshmem_LIST is set limiting which tests to run
        #   An inclusive sub-list may be specified as "first-last"  (e.g. 7-10)
        env_var = 'SST_TEST_ESshmem_LIST'
        try:
            testlist = os.environ[env_var]
            log_debug("SST_TEST_ESshmem_LIST = {0}; type = {1}".format(testlist, type(testlist)))
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

###

    def _setupESshmemTestFiles(self):
        log_debug("_setupESshmemTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.ember_ESshmem_Folder = "{0}/ember_ESshmem_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Create a clean version of the ember_ESshmem_folder Directory
        if os.path.isdir(self.ember_ESshmem_Folder):
            shutil.rmtree(self.ember_ESshmem_Folder, True)
        os.makedirs(self.ember_ESshmem_Folder)

        # Create a simlink of each file in the ember/test directory
        for f in os.listdir(self.emberelement_testdir):
            filename, ext = os.path.splitext(f)
            if ext == ".py":
                os_symlink_file(self.emberelement_testdir, self.ember_ESshmem_Folder, f)

###############################################################################

