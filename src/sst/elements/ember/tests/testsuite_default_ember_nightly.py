# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os

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
            class_inst._cleanupEmberTestFiles()
            class_inst._setupEmberTestFiles()
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################

class testcase_EmberNightly(SSTTestCase):

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

    def test_Ember_Nightly(self):
        self.Ember_test_template("test_embernightly", otherargs = "", testoutput = True)

    def test_Ember_Params(self):
        otherargs = '--verbose --model-options \"--topo=torus --shape=4x4x4 --cmdLine=\"Init\" --cmdLine=\"Allreduce\" --cmdLine=\"Fini\" \"'
        self.Ember_test_template("test_emberparams", otherargs = otherargs, testoutput = False)


#####

    def Ember_test_template(self, testcase, otherargs, testoutput):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberSweep_Folder = "{0}/embernightly_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Set the various file paths
        testDataFileName="{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/emberLoad.py".format(test_path)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs, set_cwd=self.emberSweep_Folder, mpi_out_files=mpioutfiles)

#        testing_remove_component_warning_from_file(outfile)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        if testoutput:
            cmp_result = testing_compare_diff(testDataFileName, outfile, reffile)
            if (cmp_result == False):
                diffdata = testing_get_diff_data(testDataFileName)
                log_failure(diffdata)
            self.assertTrue(cmp_result, "Diffed compared Output file {0} does not match Reference File {1}".format(outfile, reffile))

        if os_test_file(errfile, "-s"):
            log_testing_note("Ember Nightly test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))


###############################################

    def _setupEmberTestFiles(self):
        log_debug("_setupEmberNightlyTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberSweep_Folder = "{0}/embernightly_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Create a clean version of the embernightly_folder Directory
        if os.path.isdir(self.emberSweep_Folder):
            shutil.rmtree(self.emberSweep_Folder, True)
        os.makedirs(self.emberSweep_Folder)

        # Create a simlink of each file in the ember/test directory
        for f in os.listdir(self.emberelement_testdir):
            filename, ext = os.path.splitext(f)
            if ext == ".py":
                os_symlink_file(self.emberelement_testdir, self.emberSweep_Folder, f)

####

    def _cleanupEmberTestFiles(self):
        pass
    ### NOTE: The code below comes from the old bamboo testSuite_embernightly.sh
    ###       is does not seem to be doing anything on the latest test system
    ###       it is provided here as a reference if needed in the future
#    #=====================================================
#    #  A bit of code to clear out old openmpi files from /tmp
#
#        ls -ld /tmp/openmpi-sessions*/* |grep $USER > __rmlist
#        wc __rmlist
#    #    cat __rmlist | sed 10q
#
#        today=$(( 10#`date +%j` ))
#        echo "today is $today"              ## day of year
#        if [ $SST_TEST_HOST_OS_KERNEL != "Darwin" ] ; then
#        #   --This is Linux code and MacOS can't handle this date syntax--
#
#            while read -u 3 r1 r2 r3 r4 r5 mo da r8 name
#            do
#              if [ "$mo" == "Dec" ] && [ $today -lt 300 ] ; then
#                 echo found Dec
#                 echo "Remove $name"
#                 rm -rf $name
#              fi
#                  ##   c_day -   day of year for the file
#              c_day=$(( 10#`date +%j -d "$mo $da"` ))
#              c_day_plus_2=$(($c_day+2))
#              if [ $today -gt $c_day_plus_2 ] ; then
#                 echo "Remove $name"
#                 rm -rf $name
#              fi
#
#            done 3<__rmlist
#        fi
#        rm __rmlist
#    #=====================================================
