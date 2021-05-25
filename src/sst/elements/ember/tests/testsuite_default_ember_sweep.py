# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
from sst_unittest_parameterized import parameterized

import hashlib
import os

dirpath = os.path.dirname(sys.modules[__name__].__file__)
sys.path.insert(1, "{0}/../test/".format(dirpath))
import CrossProduct
from CrossProduct import *

module_init = 0
module_sema = threading.Semaphore()
sweep_sdl_file = "emberLoad.py"
sweep_test_matrix = []

################################################################################
# NOTES:
# This is a parameterized test, we setup the sweep test matrix ahead of time and
# then use the sst_unittest_parameterized module to load the list of testcases.
#
# Running specific sweeps: Setting the Environment variable
# SST_TEST_ES_LIST=9-150 will run Sweeps 9 - 150.  All other sweeps testcases
# will be skipped.
################################################################################

def build_sweep_test_matrix():
    global sweep_test_matrix
    sweep_test_matrix = []
    networks = []
    testtypes = []

    # Build the network matrix
    net = { 'topo' : 'torus',
            'args' : [
                        [ '--shape', ['2','4x4x4','8x8x8','16x16x16'] ]
                     ]
          }

    networks.append(net);

    net = { 'topo' : 'fattree',
            'args' : [
                        ['--shape',   ['9,9:9,9:18']]
                     ]
          }
    networks.append(net);

    net = { 'topo' : 'dragonfly',
            'args' : [
                        ['--shape',   ['8:8:4:8']]
                     ]
          }
    networks.append(net);

    # Build the test type matrix
    test = { 'motif' : 'AllPingPong',
             'args'  : [
                            [ 'iterations'  , ['1','10']],
                            [ 'messageSize' , ['0','1','10000','20000']]
                       ]
            }
    testtypes.append(test)

    test = { 'motif' : 'Allreduce',
             'args'  : [
                            [ 'iterations'  , ['1','10']],
                            [ 'count' , ['1']]
                       ]
            }
    testtypes.append(test)

    test = { 'motif' : 'Barrier',
             'args'  : [
                            [ 'iterations'  , ['1','10']]
                       ]
            }
    testtypes.append(test)

    test = { 'motif' : 'PingPong',
             'args'  : [
                            [ 'iterations'  , ['1','10']],
                            [ 'messageSize' , ['0','1','10000','20000']]
                       ]
            }
    testtypes.append(test)

    test = { 'motif' : 'Reduce',
             'args'  : [
                            [ 'iterations'  , ['1','10']],
                            [ 'count' , ['1']]
                       ]
            }
    testtypes.append(test)

    test = { 'motif' : 'Ring',
             'args'  : [
                            [ 'iterations'  , ['1','10']],
                            [ 'messagesize' , ['0','1','10000','20000']]
                       ]
            }
    testtypes.append(test)

    # Build the final Sweeps Tests Matrix
    index = 1
    for network in networks:
        for test in testtypes:
            for net_args in CrossProduct(network['args']) :
                for test_args in CrossProduct(test['args']):

                    hash_str = "sst --model-options=\"--topo={0} {1} --cmdLine=\\\"{2} {3}\\\"\" {4}".format(network['topo'], net_args, test['motif'], test_args, sweep_sdl_file)
                    hash_object  = hashlib.md5(hash_str.encode("UTF-8"))
                    hex_dig = hash_object.hexdigest()

                    test_data = (index, hex_dig, network['topo'], net_args, test['motif'], test_args)
                    sweep_test_matrix.append(test_data)
                    #log_debug("BUILDING SWEEP TEST MATRIX #{0} : Hex={1}; Topo{2}; Net arg = {3}; Test = {4}; Test Arg = {5}".format(index, hex_dig, network['topo'], net_args, test['motif'], test_args))
                    index += 1

################################################################################

# At startup, build the sweep test matrix
build_sweep_test_matrix()

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
            class_inst._setupEmberTestFiles()
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_EmberSweep(SSTTestCase):

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
    def test_EmberSweep(self, index, hex_dig, topo, net_args, test, test_args):
        self._checkSkipConditions(index)

        log_debug("Running Ember Sweep #{0} ({1}): {2}; Net arg = {3}; Test = {4}; Test Arg = {5}".format(index, hex_dig, topo, net_args, test, test_args))
        self.EmberSweep_test_template(index, hex_dig, topo, net_args, test, test_args)

####

    def EmberSweep_test_template(self, index, hex_dig, topo, net_args, test, test_args):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberSweep_Folder = "{0}/embersweep_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Set the various file paths
        testDataFileName="{0}".format("testEmberSweep_{0}".format(index))

        reffile = "{0}/refFiles/test_EmberSweep.out".format(test_path)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/{1}".format(test_path, sweep_sdl_file)
        testtimeout = 600

        otherargs = '--model-options=\"--topo={0} {1} --cmdLine=\\\"Init\\\" --cmdLine=\\\"{2} {3}\\\" --cmdLine=\\\"Fini\\\" \"'.format(topo, net_args, test, test_args)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, set_cwd=self.emberSweep_Folder, mpi_out_files=mpioutfiles, timeout_sec=testtimeout)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        if os_test_file(errfile, "-s"):
            log_testing_note("Ember Sweep test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        # Dig through the output file looking for Simulation is complete
        outfoundline = ""
        grepstr = 'Simulation is complete'
        with open(outfile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    outfoundline = line

        outtestresult = outfoundline != ""
        self.assertTrue(outtestresult, "Ember Sweep Test {0} - Cannot find string \"{1}\" in output file {2}".format(index, grepstr, outfile))

        reffoundline = ""
        grepstr = '{0} {1}'.format(hex_dig, outfoundline)
        with open(reffile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    reffoundline = line

        reftestresult = reffoundline != ""
        self.assertTrue(reftestresult, "Ember Sweep Test {0} - Cannot find string \"{1}\" in reference file {2}".format(index, grepstr, reffile))

        log_debug("Ember Sweep Test {0} - PASSED\n--------".format(index))

###############################################

    def _checkSkipConditions(self, sweepindex):
        # Check to see if Env Var SST_TEST_ES_LIST is set limiting which sweeps to run
        env_var = 'SST_TEST_ES_LIST'
        try:
            sweeplist = os.environ[env_var]
            log_debug("SST_TEST_ES_LIST = {0}; type = {1}".format(sweeplist, type(sweeplist)))
            index = sweeplist.find('-')
            if index > 0 and len(sweeplist) >= 3:
                startnumstr = int(sweeplist[0:index])
                stopnumstr = int(sweeplist[index+1:])
                try:
                    startnum = int(startnumstr)
                    stopnum = int(stopnumstr)
                except ValueError:
                    log_debug("Cannot decode start/stop index strings: startstr = {0}; stopstr = {1}".format(startnumstr, stopnumstr))
                    return
                log_debug("Current Sweep Index = {0}; Skip Number: startnum = {1}; stopnum = {2}".format(sweepindex, startnum, stopnum))
                if sweepindex < startnum or sweepindex > stopnum:
                    self.skipTest("Skipping Sweep #{0} - Per {1} = {2}".format(sweepindex, env_var, sweeplist))
            else:
                log_debug("Env Var {0} - not formatted correctly = {1}".format(env_var, sweeplist))
                return
        except KeyError:
            return

    def _setupEmberTestFiles(self):
        log_debug("_setupEmberSweepTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberSweep_Folder = "{0}/embersweep_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Create a clean version of the emberSweep_folder Directory
        if os.path.isdir(self.emberSweep_Folder):
            shutil.rmtree(self.emberSweep_Folder, True)
        os.makedirs(self.emberSweep_Folder)

        # Create a simlink of each file in the ember/test directory
        for f in os.listdir(self.emberelement_testdir):
            filename, ext = os.path.splitext(f)
            if ext == ".py":
                os_symlink_file(self.emberelement_testdir, self.emberSweep_Folder, f)

###############################################################################

