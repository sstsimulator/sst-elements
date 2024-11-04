# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *


class testcase_gensa_Component(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        # Put test based setup code here. it is called once before every test

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    def test_gensa_1(self):
        self.gensa_test_template("1")

#####

    def gensa_test_template(self, testcase):
        # Note: testcase param is ignored for now
        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        # Set the various file paths
        testDataFileName="test_gensa_{0}".format(testcase)

        sdlfile = "{0}/{1}.py".format(test_path, testDataFileName)
        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)

        self.run_sst(sdlfile, outfile, errfile, mpi_out_files=mpioutfiles)

        testing_remove_component_warning_from_file(outfile)

        # Perform the tests

        #   Check if any output to stderr
        if os_test_file(errfile, "-s"):
            log_testing_note("gensa test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))

        #   Check if spiking pattern exactly matches expected values
        cmp_result = True
        import OutputParser
        from OutputParser import OutputParser
        o = OutputParser()
        o.parse(outdir + "/out")
        if not self.checkColumn(o,  "0", [1,1,0,0]): cmp_result = False
        if not self.checkColumn(o,  "1", [1,1,1,0]): cmp_result = False
        if not self.checkColumn(o,  "2", [0,1,0,0]): cmp_result = False
        if not self.checkColumn(o,  "3", [0,0,1,0]): cmp_result = False
        if not self.checkColumn(o,  "4", [1,0,0,0]): cmp_result = False
        if not self.checkColumn(o,  "5", [0,1,0,0]): cmp_result = False
        if not self.checkColumn(o,  "6", [0,0,0,1]): cmp_result = False
        if not self.checkColumn(o,  "7", [0,0,1,0]): cmp_result = False
        if not self.checkColumn(o,  "8", [0,0,0,1]): cmp_result = False
        if not self.checkColumn(o,  "9", [0,1,0,0]): cmp_result = False
        if not self.checkColumn(o, "10", [1,0,0,0]): cmp_result = False
        if not self.checkColumn(o, "11", [0,0,1,0]): cmp_result = False
        if not self.checkColumn(o, "12", [0,1,0,0]): cmp_result = False
        if not self.checkColumn(o, "13", [1,1,1,0]): cmp_result = False
        if not self.checkColumn(o, "14", [1,1,0,0]): cmp_result = False
        self.assertTrue(cmp_result, "Output file {0}/out does not contain expected spike pattern".format(outdir))

    def checkColumn(self, o, index, pattern):
        c = o.getColumn(index)
        for r in range(4):
            if c.get(r) != pattern[r]: return False
        return True

