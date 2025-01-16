# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *


class testcase_memH_openMP_diropenMP(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

####

    pin_loaded = testing_is_PIN_loaded()

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompatomic(self):
        self.memH_test_template("ompatomic")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompatomicShort(self):
        self.memH_test_template("ompatomicShort")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompapi(self):
        self.memH_test_template("ompapi")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompbarrier(self):
        self.memH_test_template("ompbarrier")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompcritical(self):
        self.memH_test_template("ompcritical")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompdynamic(self):
        self.memH_test_template("ompdynamic")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompreduce(self):
        self.memH_test_template("ompreduce")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_ompthrcount(self):
        self.memH_test_template("ompthrcount")

    @unittest.skipIf(not pin_loaded, "openMP_memHierarchy: Requires PIN, but Env Var 'INTEL_PIN_DIR' is not found or path does not exist.")
    def test_diropenMP_omptriangle(self):
        self.memH_test_template("omptriangle")

####

    def memH_test_template(self, omp_app):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_diropenMP_{0}".format(omp_app)
        reffile = "{0}/refFiles/test_OMP_{1}.out".format(test_path, omp_app)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        sdlfile = "{0}/openMP/directory-8cores-2nodes.py".format(test_path)
        testtimeout = 120

        # Set the environment path to the omp application used by the SDL file
        # and verify it exists
        ompapppath = "{0}/openMP/{1}/{1}".format(test_path, omp_app)
        self.assertTrue(os.path.isfile(ompapppath), "openMP diropenMP Test; ompfile {0} - not found".format(ompapppath))
        os.environ["OMP_EXE"] = ompapppath

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles, timeout_sec=testtimeout)

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
        self.assertTrue(outtestresult, "openMP diropenMP Test - Cannot find string \"{0}\" in output file {1}".format(grepstr, outfile))

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
                    self.assertEquals(rtn.result(), 0, "openMP diropenMP Test failed running cmdline {0} - grepping reffile {1}".format(cmd, reffile))
                    refcount = int(rtn.output())

                    # Grep the out file for the count of this line occuring
                    cmd = 'grep -c "{0}" {1}'.format(testline, outfile)
                    rtn = OSCommand(cmd).run()
                    if rtn.result() == 0:
                        outcount = int(rtn.output())
                    else:
                        log_failure("FAILURE: openMP diropenMP Test failed running cmdline {0} - grepping outfile {1}".format(cmd, outfile))

                    log_debug("Testline='{0}'; refcount={1}; outcount={2}".format(testline, refcount, outcount))

                    # Compare the count
                    self.assertEquals(outcount, refcount, "openMP diropenMP testing line '{0}': outfile count = {1} does not match reffile count = {2}".format(testline, outcount, refcount))

###############################################

