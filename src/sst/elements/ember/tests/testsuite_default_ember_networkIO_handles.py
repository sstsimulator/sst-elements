

from sst_unittest import *
from sst_unittest_support import *

import os


class testcase_EmberNetworkIO_Handles(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        self._cleanupTestFiles()
        self._setupTestFiles()

    def tearDown(self):
        super(type(self), self).tearDown()

    def test_Ember_NetworkIO_Handles_open_close(self):
        self._runHandleSub("single",
                           "test_ember_networkIO_handles_open_close",
                           require_env=False)

    def test_Ember_NetworkIO_Handles_two_files(self):
        self._runHandleSub("two_files",
                           "test_ember_networkIO_handles_two_files",
                           require_env=False)

    def test_Ember_NetworkIO_Handles_short_read(self):
        self._runHandleSub("short_read",
                           "test_ember_networkIO_handles_short_read",
                           require_env=True)

    def _runHandleSub(self, mode, testcase, require_env):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        run_folder = "{0}/ember_networkIO_handles_{1}".format(tmpdir, mode)
        self._ensureFolder(run_folder)

        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO_handles.py".format(test_path)

        otherargs = '--model-options="{0}"'.format(mode)

        prior = os.environ.get("SST_EMBER_ALLOW_NETWORKIO_DENY")
        if require_env:
            os.environ["SST_EMBER_ALLOW_NETWORKIO_DENY"] = "1"
        try:
            self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                         set_cwd=run_folder, mpi_out_files=mpioutfiles)
        finally:
            if require_env:
                if prior is None:
                    os.environ.pop("SST_EMBER_ALLOW_NETWORKIO_DENY", None)
                else:
                    os.environ["SST_EMBER_ALLOW_NETWORKIO_DENY"] = prior

        combined = ""
        for path in (outfile, errfile):
            if os.path.isfile(path):
                with open(path) as f:
                    combined += f.read()
        self.assertIn(
            "Simulation is complete", combined,
            "Handles {0} sim did not complete cleanly:\n{1}".format(
                mode, combined[-2000:]))

        marker = {
            "single":     "handle-mode open_close",
            "two_files":  "handle-mode two_files",
            "short_read": "handle-mode short_read",
        }[mode]
        self.assertIn(
            marker, combined,
            "Handle-mode marker line missing in {0} output:\n{1}".format(
                mode, combined[-2000:]))

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO handles test {0} has a Non-Empty Error File {1}".format(
                    testcase, errfile))

    def _ensureFolder(self, folder):
        if os.path.isdir(folder):
            shutil.rmtree(folder, True)
        os.makedirs(folder)
        test_path = self.get_testsuite_dir()
        src_dir = "{0}/../test/".format(test_path)
        for f in os.listdir(src_dir):
            if os.path.splitext(f)[1] == ".py":
                os_symlink_file(src_dir, folder, f)

    def _setupTestFiles(self):
        pass

    def _cleanupTestFiles(self):
        pass
