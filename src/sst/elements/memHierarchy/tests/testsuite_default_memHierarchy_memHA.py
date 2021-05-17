# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os.path

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
            pass
        except:
            pass
        module_init = 1
    module_sema.release()

################################################################################
################################################################################
################################################################################

class testcase_memHierarchy_memHA(SSTTestCase):

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

    def test_memHA_BackendChaining(self):
        self.memHA_Template("BackendChaining")

    def test_memHA_BackendDelayBuffer (self):
        self.memHA_Template("BackendDelayBuffer")

    @skip_on_sstsimulator_conf_empty_str("DRAMSIM", "LIBDIR", "DRAMSIM is not included as part of this build")
    def test_memHA_BackendPagedMulti(self):
        self.memHA_Template("BackendPagedMulti", ignore_err_file=True)

    def test_memHA_BackendReorderRow(self):
        self.memHA_Template("BackendReorderRow")

    def test_memHA_BackendReorderSimple(self):
        self.memHA_Template("BackendReorderSimple")

    def test_memHA_BackendSimpleDRAM_1(self):
        self.memHA_Template("BackendSimpleDRAM_1")

    def test_memHA_BackendSimpleDRAM_2(self):
        self.memHA_Template("BackendSimpleDRAM_2")

    def test_memHA_BackendVaultSim(self):
        self.memHA_Template("BackendVaultSim", lcwc_match_allowed=True)

    def test_memHA_DistributedCaches(self):
        self.memHA_Template("DistributedCaches", lcwc_match_allowed=True)

    def test_memHA_Flushes_2(self):
        self.memHA_Template("Flushes_2")

    def test_memHA_Flushes(self):
        self.memHA_Template("Flushes")

    def test_memHA_HashXor(self):
        self.memHA_Template("HashXor")

    def test_memHA_Incoherent(self):
        self.memHA_Template("Incoherent")

    def test_memHA_Noninclusive_1(self):
        self.memHA_Template("Noninclusive_1")

    def test_memHA_Noninclusive_2(self):
        self.memHA_Template("Noninclusive_2")

    def test_memHA_PrefetchParams(self):
        self.memHA_Template("PrefetchParams")

    def test_memHA_ThroughputThrottling(self):
        self.memHA_Template("ThroughputThrottling")

    @skip_on_sstsimulator_conf_empty_str("GOBLIN_HMCSIM", "LIBDIR", "GOBLIN_HMCSIM is not included as part of this build")
    def test_memHA_BackendGoblinHMC(self):
        self.memHA_Template("BackendGoblinHMC")

    @skip_on_sstsimulator_conf_empty_str("DRAMSIM3", "LIBDIR", "DRAMSIM3 is not included as part of this build")
    def test_memHA_BackendDramsim3(self):
        self.memHA_Template("BackendDramsim3")

    @skip_on_sstsimulator_conf_empty_str("GOBLIN_HMCSIM", "LIBDIR", "GOBLIN_HMCSIM is not included as part of this build")
    def test_memHA_CustomCmdGoblin_1(self):
        self.memHA_Template("CustomCmdGoblin_1")

    @skip_on_sstsimulator_conf_empty_str("GOBLIN_HMCSIM", "LIBDIR", "GOBLIN_HMCSIM is not included as part of this build")
    def test_memHA_CustomCmdGoblin_2(self):
        self.memHA_Template("CustomCmdGoblin_2")

    @skip_on_sstsimulator_conf_empty_str("GOBLIN_HMCSIM", "LIBDIR", "GOBLIN_HMCSIM is not included as part of this build")
    def test_memHA_CustomCmdGoblin_3(self):
        self.memHA_Template("CustomCmdGoblin_3")

    def test_memHA_BackendTimingDRAM_1(self):
        self.memHA_Template("BackendTimingDRAM_1")

    def test_memHA_BackendTimingDRAM_2(self):
        self.memHA_Template("BackendTimingDRAM_2")

    def test_memHA_BackendTimingDRAM_3(self):
        self.memHA_Template("BackendTimingDRAM_3")

    def test_memHA_BackendTimingDRAM_4(self):
        self.memHA_Template("BackendTimingDRAM_4")

    @skip_on_sstsimulator_conf_empty_str("DRAMSIM", "LIBDIR", "DRAMSIM is not included as part of this build")
    @skip_on_sstsimulator_conf_empty_str("HBMDRAMSIM", "LIBDIR", "HBMDRAMSIM is not included as part of this build")
    def test_memHA_BackendHBMDramsim(self):
        self.memHA_Template("BackendHBMDramsim", ignore_err_file=True)

    @skip_on_sstsimulator_conf_empty_str("HBMDRAMSIM", "LIBDIR", "HBMDRAMSIM is not included as part of this build")
    def test_memHA_BackendHBMPagedMulti(self):
        self.memHA_Template("BackendHBMPagedMulti")

    def test_memHA_MemoryCache(self):
        self.memHA_Template("MemoryCache")

    def test_memHA_Kingsley(self):
        self.memHA_Template("Kingsley")

#####

    def memHA_Template(self, testcase, lcwc_match_allowed=False,
                       ignore_err_file=False, testtimeout=240):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Some tweeking of file names are due to inconsistencys with testcase name
        testcasename_sdl = testcase.replace("_", "-")

        # Set the various file paths
        testDataFileName=("test_memHA_{0}".format(testcase))
        sdlfile = "{0}/test{1}.py".format(test_path, testcasename_sdl)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        # Now do some checking to see if the _MC or _MR ref files exist
        if testing_check_get_num_threads() > 1:
            mc_checkfile = "{0}/refFiles/{1}_MC.out".format(test_path, testDataFileName)
            mr_checkfile = "{0}/refFiles/{1}_MR.out".format(test_path, testDataFileName)
            if os.path.exists(mc_checkfile):
                reffile = mc_checkfile
            elif os.path.exists(mr_checkfile) and testing_check_get_num_ranks() > 1:
                reffile = mr_checkfile
        fixedreffile = "{0}/{1}_fixedreffile.out".format(outdir, testDataFileName)
        tmpfile = "{0}/{1}.tmp".format(outdir, testDataFileName)
        self.grep_tmp_file = tmpfile

        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        difffile = "{0}/{1}.raw_diff".format(tmpdir, testDataFileName)

        log_debug("testcase = {0}".format(testcase))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=test_path,
                     timeout_sec=testtimeout, mpi_out_files=mpioutfiles)

        # Copy the orig reffile to the fixedreffile
        cmd = "cat {0} > {1}".format(reffile, fixedreffile)
        os.system(cmd)

        # Cleanup any DRAMSIM Debug messages; from both the output file and fixedreffile
        # (this is a bit of hammer, but it works)
        self._grep_v_cleanup_file("===== MemorySystem", outfile)
        self._grep_v_cleanup_file("===== MemorySystem", fixedreffile)
        self._grep_v_cleanup_file("TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank", outfile)
        self._grep_v_cleanup_file("TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank", fixedreffile)
        self._grep_v_cleanup_file("== Loading", outfile)
        self._grep_v_cleanup_file("== Loading", fixedreffile)
        self._grep_v_cleanup_file("DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz", outfile)
        self._grep_v_cleanup_file("DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz", fixedreffile)
        self._grep_v_cleanup_file("WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE", outfile)
        self._grep_v_cleanup_file("WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE", fixedreffile)

        if testing_check_get_num_ranks() > 1 and "BackendTimingDRAM" in testcase :
            # NOTE: Removing these two lines as we can get a 1 count diff on multi-ranks tests on DRAMSIM TESTS
            self._grep_v_cleanup_file("memory.outstanding_requests", outfile)
            self._grep_v_cleanup_file("memory.outstanding_requests", fixedreffile)
            self._grep_v_cleanup_file("memory.total_cycles", outfile)
            self._grep_v_cleanup_file("memory.total_cycles", fixedreffile)

        testing_remove_component_warning_from_file(outfile)

        # NOTE: THE PASS / FAIL EVALUATIONS ARE PORTED FROM THE SQE BAMBOO
        #       BASED testSuite_XXX.sh THESE SHOULD BE RE-EVALUATED BY THE
        #       DEVELOPER AGAINST THE LATEST VERSION OF SST TO SEE IF THE
        #       TESTS & RESULT FILES ARE STILL VALID

        # Perform the tests
        if ignore_err_file is False:
            if os_test_file(errfile, "-s"):
                log_testing_note("memHA test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        # Use diff (ignore whitespace) to see if the files are the same
        cmd = "diff -b {0} {1} > {2}".format(fixedreffile, outfile, difffile)
        filesAreTheSame = (os.system(cmd) == 0)

        if filesAreTheSame:
            log_debug(" -- Output file {0} is an exact match to (fixed) Reference File {1}".format(outfile, fixedreffile))
        else:
            # Perform the test to see if they match when sorted
            cmp_result = testing_compare_sorted_diff(testcase, outfile, fixedreffile)
            if cmp_result:
                log_debug(" -- Output file {0} is an exact match to SORTED (fixed) Reference File {1}".format(outfile, fixedreffile))
            else:
                # Not matching sorted can we check LC/WC for one last chance...
                if lcwc_match_allowed:
                    wc_out_data = os_wc(outfile, [1, 2])
                    log_debug("{0} : wc_out_data ={1}".format(outfile, wc_out_data))
                    wc_ref_data = os_wc(fixedreffile, [1, 2])
                    log_debug("{0} : wc_ref_data ={1}".format(reffile, wc_ref_data))
                    wc_line_word_count_diff = wc_out_data == wc_ref_data
                    if wc_line_word_count_diff :
                        log_debug("Line Word Count Match\n")
                        self.assertTrue(wc_line_word_count_diff, "Line & Word count between file {0} does not match Reference File {1}".format(outfile, reffile))
                else:
                    diffdata = testing_get_diff_data(testcase)
                    log_failure(diffdata)
                    self.assertTrue(cmp_result, "Sorted Output file {0} does not match Sorted Reference File {1} ".format(outfile, fixedreffile))

###

    def _grep_v_cleanup_file(self, grep_str, grep_file, out_file = None, append = False):
        cmd = 'grep -v \"{0}\" {1} > {2}'.format(grep_str, grep_file, self.grep_tmp_file)
        os.system(cmd)
        redirecttype = ">"
        if append == True:
            redirecttype = ">>"

        if out_file == None:
            cmd = "cat {0} {1} {2}".format(self.grep_tmp_file, redirecttype, grep_file)
        else:
            cmd = "cat {0} {1} {2}".format(self.grep_tmp_file, redirecttype, out_file)
        os.system(cmd)
