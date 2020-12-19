# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os
import shutil
import fnmatch
import csv

################################################################################
# Code to support a single instance module initialize, must be called setUp method

module_init = 0
module_sema = threading.Semaphore()
pin_exec_path = ""

def initializeTestModule_SingleInstance(class_inst):
    global module_init
    global module_sema

    module_sema.acquire()
    if module_init != 1:
        try:
            # Put your single instance Init Code Here
            class_inst._setup_sieve_test_files()
        except:
            pass
        module_init = 1
    module_sema.release()

###

def is_PIN_loaded():
    # Look to see if PIN is available
    pindir_found = False
    pin_path = os.environ.get('INTEL_PIN_DIRECTORY')
    if pin_path is not None:
        pindir_found = os.path.isdir(pin_path)
    log_debug("memHSieve Test - Intel_PIN_Path = {0}; Valid Dir = {1}".format(pin_path, pindir_found))
    return pindir_found

def is_PIN_Compiled():
    global pin_exec_path
    pin_crt = sst_elements_config_include_file_get_value_int("HAVE_PINCRT", 0, True)
    pin_exec = sst_elements_config_include_file_get_value_str("PINTOOL_EXECUTABLE", "", True)
    log_debug("memHSieve Test - Detected PIN_CRT = {0}".format(pin_crt))
    log_debug("memHSieve Test - Detected PIN_EXEC = {0}".format(pin_exec))
    pin_exec_path = pin_exec
    return pin_exec != ""

def is_Pin2_used():
    global pin_exec_path
    if is_PIN_Compiled():
        if "/pin.sh" in pin_exec_path:
            return True
        else:
            return False
    else:
        return False

def is_Pin3_used():
    global pin_exec_path
    if is_PIN_Compiled():
        if is_Pin2_used():
            return False
        else:
            # Make sure pin is at the end of the string
            pinstr = "/pin"
            idx = pin_exec_path.rfind(pinstr)
            if idx == -1:
                return False
            else:
                return (idx+len(pinstr)) == len(pin_exec_path)
    else:
        return False

################################################################################
################################################################################
################################################################################

class testcase_memHierarchy_memHSieve(SSTTestCase):

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
    pin_compiled = is_PIN_Compiled()
    pin_version_valid = is_Pin2_used() | is_Pin3_used()
    pin_loaded = is_PIN_loaded()

    @unittest.skipIf(not pin_compiled, "memHSieve: Requires PIN, but PinTool is not compiled with Elements. In sst_element_config.h PINTOOL_EXECUTABLE={0}".format(pin_exec_path))
    @unittest.skipIf(not pin_version_valid, "memHSieve: Requires PIN, but PinTool does not seem to be a valid version. PINTOOL_EXECUTABLE={0}".format(pin_exec_path))
    @unittest.skipIf(not pin_loaded, "memHSieve: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_memHSieve(self):
        self.memHSieve_Template("memHSieve")

#####

    def memHSieve_Template(self, testcase):

        pin2defined = is_Pin2_used()
        pin3defined = is_Pin3_used()

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        MemHElementDir = os.path.abspath("{0}/../".format(test_path))
        MemHElementSieveTestsDir = "{0}/Sieve/tests".format(self.MemHElementDir)
        testMemHSieveDir = "{0}/testmemhsieve".format(tmpdir)

        # Set the various file paths
        testDataFileName=("test_{0}".format(testcase))
        sdlfile = "{0}/sieve-test.py".format(MemHElementSieveTestsDir, testDataFileName)
        reffile = "{0}/refFiles/{1}.out".format(MemHElementSieveTestsDir, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        grep_outfile = "{0}/{1}_grep_lines_23_43.out".format(tmpdir, testDataFileName)
        grep_reffile = "{0}/{1}_grep_lines_23_43.ref".format(tmpdir, testDataFileName)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))
        log_debug("out file = {0}".format(outfile))
        log_debug("err file = {0}".format(errfile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=testMemHSieveDir, mpi_out_files=mpioutfiles)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        TestPassed = True
        TestFailureMsg = ""

        ######
        backtrace_good = False
        if pin3defined:
            # FOR PIN3
            # Make sure ANY of the backtrace*.txt files exist
            pattern = "backtrace_*.txt"
            file_found = False
            for filename in os.listdir(testMemHSieveDir):
                if fnmatch.fnmatch(filename, pattern):
                    file_found = True
                    break
            if file_found == True:
                backtrace_good = True
            else:
                TestFailureMsg += "Did not find any {0} files in directory {1}; ".format(pattern, testMemHSieveDir)
        else:
            # FOR PIN2
            # Make sure ANY of the backtrace*.txt.gz files exist
            pattern = "backtrace_*.txt.gz"
            file_found = False
            for filename in os.listdir(testMemHSieveDir):
                if fnmatch.fnmatch(filename, pattern):
                    file_found = True
                    break
            if file_found == True:
                backtrace_good = True
            else:
                TestFailureMsg += "Did not find any {0} files in directory {1}; ".format(pattern, testMemHSieveDir)

            # Unzip all expanded backtrace*.txt.gz files
            for filename in os.listdir(testMemHSieveDir):
                if fnmatch.fnmatch(filename, pattern):
                    filepath = "{0}/{1}".format(testMemHSieveDir, filename)
                    cmd = 'gzip -d {0}'.format(filepath)
                    os.system(cmd)

            # Make sure all the unzipped backtrace*.txt.gz files have data
            pattern = "backtrace_*.txt"
            file_found = False
            for filename in os.listdir(testMemHSieveDir):
                if fnmatch.fnmatch(filename, pattern):
                    file_found = True
                    filepath = "{0}/{1}".format(testMemHSieveDir, filename)
                    if os_test_file(filepath, expression='-s') == False:
                        backtrace_good = False
                        TestFailureMsg += "Unzipped Backtrace File {0} does not contain data; ".format(filename)
            if file_found == False:
                TestFailureMsg += "Did not find any unzipped {0} files in directory {1}; ".format(pattern, testMemHSieveDir)
                backtrace_good = False

        TestPassed &= backtrace_good

        ######
        # Test if ANY mallocRank files to contain words
        pattern = "mallocRank.txt*"
        file_found = False
        mallocrank_good = False
        for filename in os.listdir(testMemHSieveDir):
            if fnmatch.fnmatch(filename, pattern):
                file_found = True
                break
        if file_found == True:
            filepath = "{0}/{1}".format(testMemHSieveDir, filename)
            file = open(filepath, "rt")
            data = file.read()
            words = data.split()

            if len(words) > 0:
                mallocrank_good = True
            else:
                TestFailureMsg += "File {0} does not contain any words; ".format(filepath)
        else:
            TestFailureMsg += "Did not find any {0} files in directory {1}; ".format(pattern, testMemHSieveDir)

        TestPassed &= mallocrank_good

        ######
        # Test Statistics
        stats_good = True
        pattern = "StatisticOutput*.csv"
        stats_list = [" ReadHits", " ReadMisses", " WriteHits", " WriteMisses", \
                      " UnassociatedReadMisses", " UnassociatedWriteMisses"]
        # Look at each of the statnames in the list and if the statname exists in
        # at least one of files; also, if it exists is the data correct?
        for statname in stats_list:
            statnamefoundinfile = False
            for filename in os.listdir(testMemHSieveDir):
                if fnmatch.fnmatch(filename, pattern):
                    statfilepath = "{0}/{1}".format(testMemHSieveDir, filename)
                    # See if stat exists in the file
                    stat_check_result = self._sieve_check_stat_exists(statfilepath, statname)
                    if stat_check_result == True:
                        statnamefoundinfile = True
                        # Its found, now check its data fields
                        stat_check_result = self._sieve_check_stat_data(statfilepath, statname)
                        if stat_check_result == False:
                            stats_good = False
                            TestFailureMsg += "Statistic '{0}' in file {1} contains a 0 in one of last 3 fields; ".format(statname, statfilepath)

            # Check to see if stat name found in file
            if statnamefoundinfile == False:
                stats_good = False
                TestFailureMsg += "Statistic '{0}' Not found in any statistics file; ".format(statname, statfilepath)

        TestPassed &= stats_good

        ######
        # Test Reference File
        # This will do a grep of both the output file and ref file looking at lines 23- 43
        cmd = 'grep -w -e \"^.$\" -e \"^..$\" {0} > {1}'.format(outfile, grep_outfile)
        os.system(cmd)
        cmd = 'grep -w -e \"^.$\" -e \"^..$\" {0} > {1}'.format(reffile, grep_reffile)
        os.system(cmd)

        cmp_result = testing_compare_diff(testDataFileName, grep_outfile, grep_reffile)
        if cmp_result == False:
                TestFailureMsg += "Diffed grepped (lines 23-43) compared Output file {0} does not match Reference File {1}".format(grep_outfile, grep_reffile)
                diffdata = testing_get_diff_data(testDataFileName)
                log_failure(diffdata)

        TestPassed &= cmp_result

        ######
        # The final Test Assertion
        self.assertTrue(TestPassed, TestFailureMsg)

#######################

    def _setup_sieve_test_files(self):
        # NOTE: This routine is called a single time at module startup, so it
        #       may have some redunant
        log_debug("_setup_sieve_test_files() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.MemHElementDir = os.path.abspath("{0}/../".format(test_path))
        self.MemHElementSieveTestsDir = "{0}/Sieve/tests".format(self.MemHElementDir)
        self.testMemHSieveDir = "{0}/testmemhsieve".format(tmpdir)

        # Create a clean version of the testCramSim Directory
        if os.path.isdir(self.testMemHSieveDir):
            shutil.rmtree(self.testMemHSieveDir, True)
        os.makedirs(self.testMemHSieveDir)

        # Copy the Makefile to the test directory
        shutil.copy("{0}/Makefile".format(self.MemHElementSieveTestsDir), self.testMemHSieveDir)

        # Create a simlink of the ompsievetest.c file
        os_symlink_file(self.MemHElementSieveTestsDir, self.testMemHSieveDir, "ompsievetest.c")

        # Now run the make on it
        cmd = "make"
        rtn = OSCommand(cmd, set_cwd=self.testMemHSieveDir).run()
        log_debug("Make result = {0}; output =\n{1}".format(rtn.result(), rtn.output()))
        self.assertTrue(rtn.result() == 0, "ompsievetest.c failed to compile")

###

    def _sieve_check_stat_exists(self, statfile, stattocheck):
        # Verify that a stat exists in the file
        rtn_result = True
        found_row = False
        with open(statfile, 'rt') as csvfile:
            statreader = csv.reader(csvfile, delimiter=',')
            for row in statreader:
                if stattocheck in row:
                    found_row = True
                    break
        # Final eval to ensure we found the row
        rtn_result &= found_row
        return rtn_result

    def _sieve_check_stat_data(self, statfile, stattocheck):
        # Verify that the last 3 fields of the stat data is != 0
        rtn_result = True
        with open(statfile, 'rt') as csvfile:
            statreader = csv.reader(csvfile, delimiter=',')
            for row in statreader:
                if stattocheck in row:
                    log_debug("*** Found Stat {0} in {1}; Last 3 Data Fields = {2}, {3}, {4}".format(stattocheck, statfile, int(row[-3]), int(row[-2]), int(row[-1])))
                    rtn_result &= int(row[-3]) != 0
                    rtn_result &= int(row[-2]) != 0
                    rtn_result &= int(row[-1]) != 0
                    break
        return rtn_result



    # NOTE: This is the bash code from the bamboo test system that possibly cleans up
    #       the old ompsievetest runs.  We are not doing this unless necessary
    #       for the new frameworks
    #Remove_old_ompsievetest_task() {
    #memHS_PID=$$
    #   echo " Begin Remover -------------------"
    #ps -ef | grep ompsievetest | grep -v -e grep
    #echo "  remover  ==== $LINENO   "
    #ps -ef | grep ompsievetest | grep -v -e grep > /tmp/${memHS_PID}_omps_list
    #wc /tmp/${memHS_PID}_omps_list
    #while read -u 3 _who _anOMP _own _rest
    #do
    #    if [ $_own == 1 ] ; then
    #        echo " Attempt to remove $_anOMP "
    #        kill -9 $_anOMP
    #    fi
    #done 3</tmp/${memHS_PID}_omps_list
    #
    #rm /tmp/${memHS_PID}_omps_list
    #}



