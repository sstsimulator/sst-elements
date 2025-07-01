# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *
import os.path
import re
import filecmp

################################################################################
################################################################################

class testcase_memHierarchy_coherence(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####
    # Run all test_coherence_1core.py tests with different seeds
    def test_coherence_single_core_case0_mesi(self):
        self.memHA_Template("1core", 0, [1571], "mesi")

    def test_coherence_single_core_case1_mesi(self):
        self.memHA_Template("1core", 1, [893], "mesi")

    def test_coherence_single_core_case2_mesi(self):
        self.memHA_Template("1core", 2, [922], "mesi")

    def test_coherence_single_core_case3_mesi(self):
        self.memHA_Template("1core", 3, [24856], "mesi")

    def test_coherence_single_core_case4_mesi(self):
        self.memHA_Template("1core", 4, [11], "mesi")

    def test_coherence_single_core_case5_mesi(self):
        self.memHA_Template("1core", 5, [4836], "mesi")

    def test_coherence_single_core_case6_mesi(self):
        self.memHA_Template("1core", 6, [323], "mesi") # Protocol doesn't matter but required param

    # Run all test_coherence_2core_3level.py tets with different seeds
    def test_coherence_two_core_case0_mesi(self):
        self.memHA_Template("2core_3level", 0, [9067, 935], "mesi")

    def test_coherence_two_core_case1_mesi(self):
        self.memHA_Template("2core_3level", 1, [515, 49], "mesi")

    def test_coherence_two_core_case2_mesi(self):
        self.memHA_Template("2core_3level", 2, [9873, 5768], "mesi")

    def test_coherence_two_core_case3_mesi(self):
        self.memHA_Template("2core_3level", 3, [2315, 4623], "mesi")

    def test_coherence_two_core_case4_mesi(self):
        self.memHA_Template("2core_3level", 4, [235, 157], "mesi")

    def test_coherence_two_core_case5_mesi(self):
        self.memHA_Template("2core_3level", 5, [653, 5674], "mesi")

    def test_coherence_two_core_case6_mesi(self):
        self.memHA_Template("2core_3level", 6, [573, 1736], "mesi")

    def test_coherence_two_core_case7_mesi(self):
        self.memHA_Template("2core_3level", 7, [3475, 9774], "mesi")

    def test_coherence_two_core_case8_mesi(self):
        self.memHA_Template("2core_3level", 8, [475, 4632], "mesi")

    def test_coherence_two_core_case9_mesi(self):
        self.memHA_Template("2core_3level", 9, [583, 173], "mesi")

    def test_coherence_two_core_case10_mesi(self):
        self.memHA_Template("2core_3level", 10, [183, 586], "mesi")

    def test_coherence_two_core_case11_mesi(self):
        self.memHA_Template("2core_3level", 11, [86, 68], "mesi")

    # Run all test_coherence_4core_5level.py tets with different seeds
    def test_coherence_four_core_case0_mesi(self):
        self.memHA_Template("4core_5level", 0, [1047, 3254, 5371, 4553], "mesi")

    def test_coherence_four_core_case1_mesi(self):
        self.memHA_Template("4core_5level", 1, [0, 484, 55, 1823], "mesi")

    def test_coherence_four_core_case2_mesi(self):
        self.memHA_Template("4core_5level", 2, [966, 5957, 885, 443], "mesi")

    def test_coherence_four_core_case3_mesi(self):
        self.memHA_Template("4core_5level", 3, [5399, 2533, 433, 1834], "mesi")

    def test_coherence_none_case0(self):
        self.memHA_Template("none", 0, [476])

    def test_coherence_none_case1(self):
        self.memHA_Template("none", 1, [377])

    def test_coherence_none_case2(self):
        self.memHA_Template("none", 2, [98])

    def test_coherence_none_case3(self):
        self.memHA_Template("none", 3, [4477])

    def test_coherence_none_case4(self):
        self.memHA_Template("none", 4, [543623])
#####

    def memHA_Template(self, testcase, testnum, cpu_seeds, protocol="none",
                       llsc="yes", ignore_err_file=False, testtimeout=240):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set file paths
        test_name = "test_memHierarchy_coherence_{0}_case{1}_{2}".format(testcase, testnum, protocol)
        sdlfile = "{0}/test_coherence_{1}.py".format(test_path, testcase)
        reffile = "{0}/refFiles/{1}.out".format(test_path, test_name)
        tmpfile = "{0}/{1}.tmp".format(outdir, test_name)
        outfile = "{0}/{1}.out".format(outdir, test_name)
        errfile = "{0}/{1}.err".format(outdir, test_name)
        mpi_outfile = "{0}/{1}.testfile".format(outdir, test_name)

        # Create test arguments
        args = '--model-options="{0}'.format(testnum)
        for x in cpu_seeds:
            args = args + " " + str(x)
        if protocol != "none":
            args = args + " " + protocol + " " + llsc
        args += " " + outdir + '"'

        ## Output only in debug mode
        log_debug("testcase = {0}".format(test_name))
        log_debug("sdl file = {0}".format(sdlfile))
        log_debug("ref file = {0}".format(reffile))

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, other_args=args, set_cwd=test_path,
                     timeout_sec=testtimeout, mpi_out_files=mpi_outfile)

        # Lines to ignore
        # This is generated by SST when the number of ranks/threads > # of components
        ignore_lines = ["WARNING: No components are assigned to"]
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
                log_testing_note("memHA test {0} has a Non-Empty Error File {1}".format(test_name, errfile))

        if filesAreTheSame:
            log_debug(" -- Output file {0} passed check against the Reference File {1}".format(outfile, reffile))
        else:
            diffdata = self._prettyPrintDiffs(statDiffs, othDiffs)
            log_failure(diffdata)
            self.assertTrue(filesAreTheSame, "Output file {0} does not pass check against the Reference File {1} ".format(outfile, reffile))

        value_outfile = "{}/{}.malloc.mem".format(outdir, test_name)
        value_reffile = "{}/refFiles/{}.malloc.mem".format(test_path, test_name)
        valueCheck = filecmp.cmp(value_outfile, value_reffile)

        self.assertTrue(valueCheck, "Output data value file {0} does not pass check against the reference file {1} ".format(value_outfile, value_reffile))

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
        cons_accum = re.compile(r' ([\w.]+)\.(\w+) : Accumulator : Sum.(\w+) = (\d+); SumSQ.\w+ = (\d+); Count.\w+ = (\d+); Min.\w+ = (\d+); Max.\w+ = (\d+);')
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
