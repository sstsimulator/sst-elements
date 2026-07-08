# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import csv
import glob
import os


class testcase_EmberNetworkIO(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        self._cleanupEmberTestFiles()
        self._setupEmberTestFiles()

    def tearDown(self):
        super(type(self), self).tearDown()

#####

    def test_Ember_NetworkIO(self):
        # The canonical NetworkIO smoke (testIO.py) takes no model options:
        # the configuration is fixed inside the .py (4-node Torus, 2 I/O
        # nodes, EmberMPIJob+TestNetworkIO motif).  This testsuite simply
        # runs it and matches filtered stdout.
        self.Ember_test_template("test_ember_networkIO", otherargs="", testoutput=True)

#####

    def test_Ember_NetworkIO_stats(self):
        # Stats regression: testIO.py with a model-option enables the
        # deterministic stats mode (seeded MarsagliaRNG + CSV stat emit
        # covering all firefly.nic and SimpleSSD AccumulatorStatistics).
        # We exact-diff the produced CSV against a refFile, normalising
        # only the per-rank column so np=1 and np=2 reuse the same ref.
        self.Ember_stats_template("test_ember_networkIO_stats")

#####

    def Ember_test_template(self, testcase, otherargs, testoutput):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberNetworkIO_Folder = "{0}/ember_networkIO_folder".format(tmpdir)

        # Set the various file paths
        testDataFileName = "{0}".format(testcase)

        reffile = "{0}/refFiles/{1}.out".format(test_path, testDataFileName)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/testIO.py".format(test_path)

        # Run SST
        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                     set_cwd=self.emberNetworkIO_Folder, mpi_out_files=mpioutfiles)

        if testoutput:
            # SimpleSSD enqueue rotation and per-iteration RNG choice make
            # the absolute timing values jitter at the sub-microsecond
            # level run-to-run.  Strip the numeric "<X.YYY> us" suffix so
            # the diff is content-only.
            #
            # When the test runs under multiple MPI ranks the motif
            # latency lines come from the ember rank while the
            # "Simulation is complete" line comes from the core rank,
            # and the relative order is not deterministic. Sort the
            # filtered lines so the comparison is order-agnostic for
            # any rank count.
            timing_filter = RemoveRegexFromLineFilter(r"[0-9]+\.[0-9]+ us")
            cmp_result = testing_compare_filtered_diff(
                testDataFileName, outfile, reffile, sort=True, filters=timing_filter
            )
            if cmp_result is False:
                diffdata = testing_get_diff_data(testDataFileName)
                log_failure(diffdata)
            self.assertTrue(
                cmp_result,
                "Diffed compared Output file {0} does not match Reference File {1}".format(outfile, reffile),
            )

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile)
            )


###############################################

    def Ember_stats_template(self, testcase):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberNetworkIO_Folder = "{0}/ember_networkIO_folder".format(tmpdir)
        stats_csv_name = "okmar_stats.csv"

        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO.py".format(test_path)
        reffile = "{0}/refFiles/{1}.csv".format(test_path, testcase)

        self.run_sst(
            sdlfile, outfile, errfile,
            other_args='--model-options="{0}"'.format(stats_csv_name),
            set_cwd=self.emberNetworkIO_Folder,
            mpi_out_files=mpioutfiles,
        )

        # The stats CSV lands inside set_cwd. Multi-rank runs shard the
        # filename as <stem>_<rank>.csv; single-rank uses <stem>.csv.
        stem, ext = os.path.splitext(stats_csv_name)
        candidates = sorted(
            glob.glob("{0}/{1}{2}".format(self.emberNetworkIO_Folder, stem, ext))
            + glob.glob("{0}/{1}_*{2}".format(self.emberNetworkIO_Folder, stem, ext))
        )
        self.assertTrue(
            len(candidates) > 0,
            "No stats CSV produced (looked for {0}/{1}{2} or shards)".format(
                self.emberNetworkIO_Folder, stem, ext
            ),
        )

        # Normalise: strip the Rank column (col index 5) so np=1 and np=2
        # both produce the same canonical form. SimTime (col 4) is kept
        # because it is deterministic at 7399000 ns under the seeded run.
        observed_rows = []
        for path in candidates:
            with open(path, newline="") as f:
                reader = csv.reader(f, skipinitialspace=True)
                for i, row in enumerate(reader):
                    if i == 0:
                        continue
                    if len(row) != 11:
                        continue
                    observed_rows.append(
                        [row[0], row[1], row[2], row[3], row[4]] + row[6:11]
                    )
        observed_rows.sort()

        with open(reffile, newline="") as f:
            reader = csv.reader(f, skipinitialspace=True)
            ref_rows = []
            for i, row in enumerate(reader):
                if i == 0:
                    continue
                if len(row) != 10:
                    continue
                ref_rows.append(row)

        self.assertEqual(
            len(observed_rows), len(ref_rows),
            "Stats CSV row count {0} != ref {1}".format(
                len(observed_rows), len(ref_rows)),
        )
        for obs, ref in zip(observed_rows, ref_rows):
            self.assertEqual(
                obs, ref,
                "Stats row mismatch:\n  observed: {0}\n  ref:      {1}".format(obs, ref),
            )

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO stats test {0} has a Non-Empty Error File {1}".format(
                    testcase, errfile
                )
            )

###############################################

    def _setupEmberTestFiles(self):
        log_debug("_setupEmberTestFiles() Running")
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberNetworkIO_Folder = "{0}/ember_networkIO_folder".format(tmpdir)
        self.emberelement_testdir = "{0}/../test/".format(test_path)

        # Create a clean version of the ember_networkIO_folder directory
        if os.path.isdir(self.emberNetworkIO_Folder):
            shutil.rmtree(self.emberNetworkIO_Folder, True)
        os.makedirs(self.emberNetworkIO_Folder)

        # Create a symlink of each .py file in the ember/test directory
        for f in os.listdir(self.emberelement_testdir):
            filename, ext = os.path.splitext(f)
            if ext == ".py":
                os_symlink_file(self.emberelement_testdir, self.emberNetworkIO_Folder, f)

####

    def _cleanupEmberTestFiles(self):
        pass
