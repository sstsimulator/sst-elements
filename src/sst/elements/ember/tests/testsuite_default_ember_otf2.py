# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os


class testcase_EmberOTF2(SSTTestCase):

    otf2_support = sst_elements_config_include_file_get_value_int("HAVE_OTF2", 0, True) > 0

    def setUp(self):
        super(type(self), self).setUp()
        self._cleanupEmberTestFiles()
        self._setupEmberTestFiles()

    def tearDown(self):
        # Put test based teardown code here. it is called once after every test
        super(type(self), self).tearDown()

#####

    @unittest.skipIf(not otf2_support, "Ember: Requires OTF2, but sst-elements was not compiled with OTF2 support.")
    def test_Ember_OTF2(self):
        otherargs = '--exit-after=20s --model-options \'--topo=fattree --shape=4,4:1 --cmdLine=\"Init\" --cmdLine=\"OTF2 tracePrefix={0}\" --cmdLine=\"Fini\" \' '
        self.Ember_test_template("test_emberotf2", otherargs = otherargs, testoutput = True)

#####

    def Ember_test_template(self, testcase, otherargs, testoutput):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberOTF2_Folder = "{0}/emberotf2_folder".format(tmpdir)

        # Set the various file paths
        testDataFileName="{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/emberLoad.py".format(test_path)
        tracefile = "{0}/addFiles/{1}/traces.otf2".format(test_path, testDataFileName)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs.format(tracefile), set_cwd=self.emberOTF2_Folder, mpi_out_files=mpioutfiles)

        if testoutput:
            cmp_result = testing_compare_filtered_diff(testDataFileName, outfile, reffile, filters=StartsWithFilter("EMBER: Motif='OTF2"))
            if (cmp_result == False):
                diffdata = testing_get_diff_data(testDataFileName)
                log_failure(diffdata)
            self.assertTrue(cmp_result, "Diffed compared Output file {0} does not match Reference File {1}".format(outfile, reffile))

        if os_test_file(errfile, "-s"):
            log_testing_note("Ember OTF2 test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile))


###############################################

    def _setupEmberTestFiles(self):
        log_debug("_setupEmberTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberOTF2_Folder = "{0}/emberotf2_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Create a clean version of the emberotf2_folder Directory
        if os.path.isdir(self.emberOTF2_Folder):
            shutil.rmtree(self.emberOTF2_Folder, True)
        os.makedirs(self.emberOTF2_Folder)

        # Create a simlink of each file in the ember/test directory
        for f in os.listdir(self.emberelement_testdir):
            filename, ext = os.path.splitext(f)
            if ext == ".py":
                os_symlink_file(self.emberelement_testdir, self.emberOTF2_Folder, f)

####

    def _cleanupEmberTestFiles(self):
        pass

