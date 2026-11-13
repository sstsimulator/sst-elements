

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil


class testcase_EmberNetworkIO_Async(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        self._cleanupTestFiles()
        self._setupTestFiles()

    def tearDown(self):
        super(type(self), self).tearDown()

    def test_Ember_NetworkIO_Async_iread_smoke(self):
        self._runAsyncSub("single",
                          "test_ember_networkIO_async_iread_smoke",
                          require_env=False)

    def test_Ember_NetworkIO_Async_waitall_16(self):
        self._runAsyncSub("waitall_16",
                          "test_ember_networkIO_async_waitall_16",
                          require_env=False)

    def test_Ember_NetworkIO_Async_cancel_inflight(self):
        self._runAsyncSub("cancel_inflight",
                          "test_ember_networkIO_async_cancel_inflight",
                          require_env=False)

    def _runAsyncSub(self, mode, testcase, require_env):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        run_folder = "{0}/ember_networkIO_async_{1}".format(tmpdir, mode)
        self._ensureFolder(run_folder)

        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO_async.py".format(test_path)

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
            "Async {0} sim did not complete cleanly:\n{1}".format(
                mode, combined[-2000:]))

        op_marker = {
            "single":          "async-mode async",
            "waitall_16":      "async-mode async",
            "cancel_inflight": "async-mode async_cancel",
        }[mode]
        self.assertIn(
            op_marker, combined,
            "Async-mode marker line missing in {0} output:\n{1}".format(
                mode, combined[-2000:]))

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO async test {0} has a Non-Empty Error File {1}".format(
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
