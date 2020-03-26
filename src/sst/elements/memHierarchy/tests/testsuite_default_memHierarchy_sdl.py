# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

################################################################################

class testcase_memHierarchy_second_debug(SSTTestCase):
    def test_memHierarchy_debug_1(self):
        pass

    def test_memHierarchy_debug_2(self):
        pass

################################################################################

class testcase_memHierarchy_Component(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_memHierarchy_sdl_1(self):
        #  sdl-1   Simple CPU + 1 level cache + Memory
        self.memHierarchy_Template("sdl-1", 500)

    def test_memHierarchy_sdl_2(self):
        #  sdl-2  Simple CPU + 1 level cache + DRAMSim Memory
        self.memHierarchy_Template("sdl-2", 500)

    def test_memHierarchy_sdl_3(self):
        #  sdl-3  Simple CPU + 1 level cache + DRAMSim Memory (alternate block size)
        self.memHierarchy_Template("sdl-3", 500)

    def test_memHierarchy_sdl2_1(self):
        #  sdl2-1  Simple CPU + 2 levels cache + Memory
        self.memHierarchy_Template("sdl2-1", 500)

    def test_memHierarchy_sdl3_1(self):
        #  sdl3-1  2 Simple CPUs + 2 levels cache + Memory
        self.memHierarchy_Template("sdl3-1", 500)

    def test_memHierarchy_sdl3_2(self):
        #  sdl3-2  2 Simple CPUs + 2 levels cache + DRAMSim Memory
        self.memHierarchy_Template("sdl3-2", 500)

    def test_memHierarchy_sdl3_3(self):
        self.memHierarchy_Template("sdl3-3", 500)

    def test_memHierarchy_sdl4_1(self):
        self.memHierarchy_Template("sdl4-1", 500)

    @skipOnSSTSimulatorConfEmptyStr ("DRAMSIM", "LIBDIR", "DRAMSIM is not included as part of this build")
    def test_memHierarchy_sdl4_2_dramsim(self):
        self.memHierarchy_Template("sdl4-2", 500)

    @skipOnSSTSimulatorConfEmptyStr ("RAMULATOR", "LIBDIR", "RAMULATOR is not included as part of this build")
    def test_memHierarchy_sdl4_2_ramulator(self):
        self.memHierarchy_Template("sdl4-2-ramulator", 500)

    @skipOnSSTSimulatorConfEmptyStr ("DRAMSIM", "LIBDIR", "DRAMSIM is not included as part of this build")
    def test_memHierarchy_sdl5_1_dramsim(self):
        self.memHierarchy_Template("sdl5-1", 500)

    @skipOnSSTSimulatorConfEmptyStr ("RAMULATOR", "LIBDIR", "RAMULATOR is not included as part of this build")
    def test_memHierarchy_sdl5_1_ramulator(self):
        if get_testing_num_threads() > 1:
            self.memHierarchy_Template("sdl5-1-ramulator_MC", 500)
        else:
            self.memHierarchy_Template("sdl5-1-ramulator", 500)

    def test_memHierarchy_sdl8_1(self):
        self.memHierarchy_Template("sdl8-1", 500)

    def test_memHierarchy_sdl8_3(self):
        self.memHierarchy_Template("sdl-3", 500)

    def test_memHierarchy_sdl8_4(self):
        self.memHierarchy_Template("sdl8-4", 500)

    def test_memHierarchy_sdl9_1(self):
        if get_testing_num_threads() > 1:
            self.memHierarchy_Template("sdl9-1_MC", 500)
        else:
            self.memHierarchy_Template("sdl9-1", 500)

    def test_memHierarchy_sdl9_2(self):
        self.memHierarchy_Template("sdl9-2", 500)

#####

    def memHierarchy_Template(self, testcase, tolerance):
        # Get the path to the test files
        test_path = self.get_testsuite_dir()

        # Some tweeking of file names are due to inconsistencys with testcase name
        testcasename_sdl = testcase.replace("_MC", "")
        testcasename_out = testcase.replace("-", "_")

        # Set the various file paths
        sdlfile = "{0}/{1}.py".format(test_path, testcasename_sdl)
        testDataFileName=("test_memHierarchy_{0}".format(testcasename_out))
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(get_test_output_run_dir(), testDataFileName)
        errfile = "{0}/{1}.err".format(get_test_output_run_dir(), testDataFileName)
        tmpfile = "{0}/{1}.tmp".format(get_test_output_tmp_dir(), testDataFileName)
        difffile = "{0}/{1}.raw_diff".format(get_test_output_tmp_dir(), testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(get_test_output_run_dir(), testDataFileName)

        # Delete any leftover dramsim*.log files that might have been left over
        cmd = "rm -f {0}/dramsim*.log".format(test_path)
        os.system(cmd)

        # See if sdl file is using DramSimm
        cmd = "grep backend {0} | grep dramsim > /dev/null".format(sdlfile)
        usingDramSim = (os.system(cmd) == 0)

        # Run SST in the tests directory
        self.run_sst(sdlfile, outfile, errfile, set_cwd=test_path, mpi_out_files=mpioutfiles)

        # Get a count of "not aligned messages
        cmd = "grep -c 'not aligned to the request size' {0} >> /dev/null".format(errfile)
        notAlignedCount = os.system(cmd)

        # if not multi-rank run, append the errfile onto the output file
        if get_testing_num_ranks() < 2:
            cmd = "grep -v 'not aligned to the request size' {0} >> {1}".format(errfile, outfile)
            os.system(cmd)

        # Post processing clean up output file
        cmd = "grep -v ^cpu.*: {0} > {1}".format(outfile, tmpfile)
        os.system(cmd)
        cmd = "grep -v 'not aligned to the request size' {0} > {1}".format(tmpfile, outfile)
        os.system(cmd)

        # This may be needed in the future
        #remove_component_warning()

        # Use diff (ignore whitespace) to see if the files are the same
        cmd = "diff -b {0} {1} > {2}".format(reffile, outfile, difffile)
        filesAreTheSame = (os.system(cmd) == 0)

        if not filesAreTheSame:
            # Perform the test to see if they match when sorted
            cmp_result = compare_sorted(testcase, outfile, reffile)
            self.assertTrue(cmp_result, "Output/Compare file {0} does not match Reference File {1}".format(outfile, reffile))

        # Make sure the simulation completed
        cmd = "grep 'Simulation is complete, simulated time:' {0} >> /dev/null".format(outfile)
        foundEndMsg = (os.system(cmd) == 0)
        self.assertTrue(foundEndMsg, "Did not find 'Simulation is complete, simulated time:' in output file")
