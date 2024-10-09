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
        self.memHA_Template("BackendVaultSim")

    def test_memHA_DistributedCaches(self):
        self.memHA_Template("DistributedCaches")

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
        self.memHA_Template("BackendGoblinHMC", testtimeout=400)

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
    
    def test_memHA_ScratchCache_1(self):
        self.memHA_Template("ScratchCache_1")
    
    def test_memHA_ScratchCache_2(self):
        self.memHA_Template("ScratchCache_2")
    
    def test_memHA_ScratchCache_3(self):
        self.memHA_Template("ScratchCache_3")
    
    def test_memHA_ScratchCache_4(self):
        self.memHA_Template("ScratchCache_4")
    
    def test_memHA_ScratchDirect(self):
        self.memHA_Template("ScratchDirect")
    
    def test_memHA_ScratchNetwork(self):
        self.memHA_Template("ScratchNetwork")
    
    def test_memHA_StdMem(self):
        self.memHA_Template("StdMem")
    
    def test_memHA_StdMem_flush(self):
        self.memHA_Template("StdMem_flush")
    
    def test_memHA_StdMem_nic(self):
        self.memHA_Template("StdMem_nic")
    
    def test_memHA_StdMem_noninclusive(self):
        self.memHA_Template("StdMem_noninclusive")
    
    def test_memHA_StdMem_mmio(self):
        self.memHA_Template("StdMem_mmio")
    
    def test_memHA_StdMem_mmio2(self):
        self.memHA_Template("StdMem_mmio2")
    
    def test_memHA_StdMem_mmio3(self):
        self.memHA_Template("StdMem_mmio3")
#####

    def memHA_Template(self, testcase,
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
        
        tmpfile = "{0}/{1}.tmp".format(outdir, testDataFileName)

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
        
        # Lines to ignore
        # These are generated by DRAMSim
        ignore_lines = ["===== MemorySystem"]
        ignore_lines.append("TOTAL_STORAGE : 2048MB | 1 Ranks | 16 Devices per rank") 
        ignore_lines.append("== Loading")
        ignore_lines.append("DRAMSim2 Clock Frequency =1Hz, CPU Clock Frequency=1Hz")
        ignore_lines.append("WARNING: UNKNOWN KEY 'DEBUG_TRANS_FLOW' IN INI FILE")
        # This is generated by SST when the number of ranks/threads > # of components
        ignore_lines.append("WARNING: No components are assigned to")
        #These are warnings/info generated by SST/memH in debug mode
        ignore_lines.append("Notice: memory controller's region is larger than the backend's mem_size")
        ignore_lines.append("Region: start=")

        # Statistics that count occupancy on each cycle sometimes diff in parallel execution
        # due to the synchronization interval sometimes allowing the clock to run ahead a cycle or so
        tol_stats = { "outstanding_requests" : [0, 0, 20, 0, 0], # Only diffs in number of cycles
                      "total_cycles" : [20, 'X', 20, 20, 20],    # This stat is set once at the end of sim. May vary in all fields
                      "MSHR_occupancy" : [0, 0, 20, 0, 0] }      # Only diffs in number of cycles

        filesAreTheSame, statDiffs, othDiffs = testing_stat_output_diff(outfile, reffile, ignore_lines, tol_stats, True)

        # Perform the tests
        if ignore_err_file is False:
            if os_test_file(errfile, "-s"):
                log_testing_note("memHA test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        if filesAreTheSame:
            log_debug(" -- Output file {0} passed check against the Reference File {1}".format(outfile, reffile))
        else:
            diffdata = self._prettyPrintDiffs(statDiffs, othDiffs)
            log_failure(diffdata)
            self.assertTrue(filesAreTheSame, "Output file {0} does not pass check against the Reference File {1} ".format(outfile, reffile))

###
    # Remove lines containing any string found in 'remove_strs' from in_file
    # If out_file != None, output is out_file
    # Otherwise, in_file is overwritten
    def _remove_lines_cleanup_file(self, remove_strs, in_file, out_file = None):
        with open(in_file, 'r') as fp:
            lines = fp.readlines()
        
        if out_file == None:
            out_file = in_file
        
        with open(out_file, 'w') as fp:
            fp.truncate(0)
            for line in lines:
                skip = False
                for search in remove_strs:
                    if search in line:
                        skip = True
                        continue
                if not skip:
                    fp.write(line)

    ####################################
    # TODO move these two functions to the Core test frameworks utilities once they have matured
    # These are used to diff statistic output files with some extra checking abilities
    ####################################

    
    # Return a parsed statistic or 'None' if the line is not a statistic
    # Currently handles console output format only and integer statistic formats
    # Stats are parsed into [component_name, stat_name, sum, sumSQ, count, min, max]
    def _is_stat(self, line):
        cons_accum = re.compile(' ([\w.]+)\.(\w+) : Accumulator : Sum.(\w+) = (\d+); SumSQ.\w+ = (\d+); Count.\w+ = (\d+); Min.\w+ = (\d+); Max.\w+ = (\d+);')
        m = cons_accum.match(line)
        if m == None:
            return None
        stat = [m.group(1), m.group(2)]
        if 'u' in m.group(3) or 'i' in m.group(3):
            stat.append(int(m.group(4)))
            stat.append(int(m.group(5)))
            stat.append(int(m.group(6)))
            stat.append(int(m.group(7)))
            stat.append(int(m.group(8)))
        else:
            print("Stat parsing is not supported for datatype " + m.group(3))
            sys.exit(1)
        return stat

    # Diff 'ref' against 'out' with special handling based on the other input options
    # Input: ref - Reference filename
    # Input: out - Output filename to diff against
    # Input: ignore_lines - list of strings to ignore in the ref and out files. Any line that contains one of these strings will be ignored.
    # Input: tol_stats - statistics to diff within a tolerance. 
    #                    A map of statistic name to a list of tolerances on each field (sum, sumSq, count, min, max). 
    #                    'X' indicates don't care. All others are treated as a +/- on the ref value
    # Input: new_stats - if True, the diff will ignore any new statistics in the out file that don't exist in the ref file
    # Ouput: pass - whether the test passed (no diffs) or not
    # Output: stat_diffs - list of diffs on statistics with '<' indicating ref file lines and '>' indicating out file lines 
    # Output: oth_diffs - list of diffs on non-statistic lines with '<' indicating ref file lines and '>' indicating out file lines
    def _diffStatFiles(self, ref, out, ignore_lines, tol_stats, new_stats):
        with open(ref, 'r') as fp:
            ref_lines = fp.read().splitlines()

        with open(out, 'r') as fp:
            out_lines = fp.read().splitlines()

        # Sort by stat/not stat
        ref_stats = []
        ref_oth = []
        out_stats = []
        out_oth = []

        for line in ref_lines:
            stat = self._is_stat(line)
            if stat != None:
                ref_stats.append(stat)
            elif not any(x in line for x in ignore_lines):
                ref_oth.append(line)

        for line in out_lines:
            stat = self._is_stat(line)
            if stat == None:
                if line in ref_oth:
                    ref_oth.remove(line) # Didn't diff
                elif not any(x in line for x in ignore_lines):
                    out_oth.append(line) # Will diff
            else:
                # Filter out exact matches immediately
                if stat in ref_stats:
                    ref_stats.remove(stat)
                # Filter out new statistics (stats not in ref file) if requested
                elif not new_stats or any((row[0] == stat[0] and row[1] == stat[1]) for row in ref_stats):
                    # Check for diff within tolerances
                    if stat[1] in tol_stats:
                        found = False
                        for s in ref_stats:
                            if s[0] == stat[0] and s[1] == stat[1]:
                                ref = s
                                found = True
                                break
                        if found:
                            diffs = False
                            tol = tol_stats[stat[1]]
                            for i, t in enumerate(tol):
                                if t != 'X' and ((ref[2+i] - t) > stat[2+i] or (ref[2+i] + t) < stat[2+i]):
                                    diffs = True
                                    out_stats.append(stat)
                                    break
                            if not diffs:
                                ref_stats.remove(ref)
                        else: # Tolerance on stat but doesn't match a stat in ref 
                            out_stats.append(stat)
                    else: # No tolerance on stat and doesn't match a stat in ref
                        out_stats.append(stat)
    
        stat_diffs = [ ['<',x[0],x[1],x[2],x[3],x[4],x[5],x[6]] for x in ref_stats ]
        stat_diffs += [ ['>',x[0],x[1],x[2],x[3],x[4],x[5],x[6]] for x in out_stats ]
    
        oth_diffs = [ ['<',x] for x in ref_oth ]
        oth_diffs += [ ['>',x] for x in out_oth ]
    
        if len(stat_diffs) > 0 or len(oth_diffs) > 0:
            return False, stat_diffs, oth_diffs
        else:
            return True, stat_diffs, oth_diffs
    
    def _prettyPrintDiffs(self, stat_diff, oth_diff):
        out = ""
        if len(stat_diff) != 0:
            out = "Statistic diffs:\n"
            for x in stat_diff:
                out += (x[0] + " " + ",".join(str(y) for y in x[1:]) + "\n")
        
        if len(oth_diff) != 0:
            out += "Non-statistic diffs:\n"
            for x in oth_diff:
                out += x[0] + " " + x[1] + "\n"

        return out
