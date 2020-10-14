# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import hashlib

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()
sweep_sdl_file = "emberLoad.py"

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        # Put your single instance Init Code Here
        class_inst.networks = []
        class_inst.tests = []
        class_inst.final_test_matrix = []
        class_inst._build_network_parameters()
        class_inst._build_test_parameters()
        class_inst._build_final_test_matrix()
        class_inst._setupEmberTestFiles()

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

#####

    def test_EmberSweep(self):
        for testdata in self.final_test_matrix:
            index = testdata[0]
            hex_dig = testdata[1]
            topo = testdata[2]
            net_args = testdata[3]
            test = testdata[4]
            test_args = testdata[5]
            if index >= 3 and index <= 5:
                log_debug("Running Ember Sweep Test {0} ({1}): {2}; Net arg = {3}; Test = {4}; Test Arg = {5}".format(index, hex_dig, topo, net_args, test, test_args))
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
        testDataFileName="{0}".format("testEemberSweep_{0}".format(index))

        reffile = "{0}/refFiles/test_EmberSweep.out".format(test_path)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/{1}".format(test_path, sweep_sdl_file)
        testtimeout = 300

        otherargs = '--model-options=\"--topo={0} {1} --cmdLine=\\\"Init\\\" --cmdLine=\\\"{2} {3}\\\" --cmdLine=\\\"Fini\\\" \"'.format(topo, net_args, test, test_args)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, set_cwd=self.emberSweep_Folder, mpi_out_files=mpioutfiles, timeout_sec=testtimeout)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        self.assertFalse(os_test_file(errfile, "-s"), "Ember Sweep Test {0} has Non-empty Error File {1}".format(index, errfile))

        # Dig through the output file looking for Simulation is complete
        outfoundline = ""
        grepstr = 'Simulation is complete'
        with open(outfile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    outfoundline = line

        outtestresult = outfoundline is not ""
        self.assertTrue(outtestresult, "Ember Sweep Test {0} - Cannot find string \"{1}\" in output file {2}".format(index, grepstr, outfile))

        reffoundline = ""
        grepstr = '{0} {1}'.format(hex_dig, outfoundline)
        with open(reffile, 'r') as f:
            for line in f.readlines():
                if grepstr in line:
                    reffoundline = line

        reftestresult = reffoundline is not ""
        self.assertTrue(reftestresult, "Ember Sweep Test {0} - Cannot find string \"{1}\" in reference file {2}".format(index, grepstr, outfile))

        log_debug("Ember Sweep Test {0} - PASSED\n--------".format(index))

###############################################

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

####

    def _build_network_parameters(self):
        self.networks = []
        net = { 'topo' : 'torus',
                'args' : [
                            [ '--shape', ['2','4x4x4','8x8x8','16x16x16'] ]
                         ]
              }

        self.networks.append(net);

        net = { 'topo' : 'fattree',
                'args' : [
                            ['--shape',   ['9,9:9,9:18']]
                         ]
              }
        self.networks.append(net);

        net = { 'topo' : 'dragonfly',
                'args' : [
                            ['--shape',   ['8:8:4:8']]
                         ]
              }
        self.networks.append(net);

####

    def _build_test_parameters(self):
        self.tests = []
        test = { 'motif' : 'AllPingPong',
                 'args'  : [
                                [ 'iterations'  , ['1','10']],
                                [ 'messageSize' , ['0','1','10000','20000']]
                           ]
                }
        self.tests.append(test)

        test = { 'motif' : 'Allreduce',
                 'args'  : [
                                [ 'iterations'  , ['1','10']],
                                [ 'count' , ['1']]
                           ]
                }
        self.tests.append(test)

        test = { 'motif' : 'Barrier',
                 'args'  : [
                                [ 'iterations'  , ['1','10']]
                           ]
                }
        self.tests.append(test)

        test = { 'motif' : 'PingPong',
                 'args'  : [
                                [ 'iterations'  , ['1','10']],
                                [ 'messageSize' , ['0','1','10000','20000']]
                           ]
                }
        self.tests.append(test)

        test = { 'motif' : 'Reduce',
                 'args'  : [
                                [ 'iterations'  , ['1','10']],
                                [ 'count' , ['1']]
                           ]
                }
        self.tests.append(test)

        test = { 'motif' : 'Ring',
                 'args'  : [
                                [ 'iterations'  , ['1','10']],
                                [ 'messagesize' , ['0','1','10000','20000']]
                           ]
                }
        self.tests.append(test)

####

    def _build_final_test_matrix(self):
        self.final_test_matrix = []
        index = 1
        for network in self.networks:
            for test in self.tests:
                for net_args in CrossProduct(network['args']) :
                    for test_args in CrossProduct(test['args']):

                        hash_str = "sst --model-options=\"--topo={0} {1} --cmdLine=\\\"{2} {3}\\\"\" {4}".format(network['topo'], net_args, test['motif'], test_args, sweep_sdl_file)
                        hash_object  = hashlib.md5(hash_str.encode("UTF-8"))
                        hex_dig = hash_object.hexdigest()

                        test_data = (index, hex_dig, network['topo'], net_args, test['motif'], test_args)
                        self.final_test_matrix.append(test_data)
                        #log_debug("BUILDING TEST MATRIX #{0} : Hex={[1}; Topo{2}; Net arg = {3}; Test = {4}; Test Arg = {5}".format(index, hex_dig, network['topo'], net_args, test['motif'], test_args))
                        index += 1

###############################################################################

class CrossProduct:
    def __init__(self,xxx):
        self.xxx = xxx
        self.cnts = []
        for i, x in enumerate( xxx ) :
            self.cnts.insert( i, len( x[1] ) )

    def __iter__(self):
        return self

    def __next__(self):
        if self.cnts[0] == 0:
            raise StopIteration
        else:
            tmp = ''
            for i, x in enumerate(self.xxx) :
                tmp += "{0}={1} ".format( x[0], x[1][ self.cnts[i] - 1 ] )

            for i in reversed(range( 0, len(self.xxx) )):
                self.cnts[i] -= 1
                if  i == 0 and self.cnts[i] == 0:
                    break

                if self.cnts[i] > 0 :
                    break
                else :
                    self.cnts[i] = len(self.xxx[i][1])

            return tmp

    def next(self):
        return self.__next__()
