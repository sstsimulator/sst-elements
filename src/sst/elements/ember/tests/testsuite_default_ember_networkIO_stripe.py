# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import csv
import glob
import os
import statistics


STRIPE_COV_ITERATIONS = 1000
STRIPE_COV_TOLERANCE = 0.05


class testcase_EmberNetworkIOStripe(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        self._cleanupEmberTestFiles()
        self._setupEmberTestFiles()

    def tearDown(self):
        super(type(self), self).tearDown()

#####

    def test_Ember_NetworkIO_Stripe_block(self):
        self.Ember_stripe_template("block")

    def test_Ember_NetworkIO_Stripe_rr(self):
        self.Ember_stripe_template("rr")

    def test_Ember_NetworkIO_Stripe_hash(self):
        self.Ember_stripe_template("hash")

    def test_Ember_NetworkIO_Stripe_block_balance(self):
        self.Ember_stripe_balance_template("block")

    def test_Ember_NetworkIO_Stripe_rr_balance(self):
        self.Ember_stripe_balance_template("rr")

    def test_Ember_NetworkIO_Stripe_hash_balance(self):
        self.Ember_stripe_balance_template("hash")

#####

    def Ember_stripe_template(self, policy):

        # Get the path to the test files
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberNetworkIO_Folder = "{0}/ember_networkIO_folder".format(tmpdir)

        # All three policies (block / rr / hash) produce identical post-filter
        # stdout (same iteration count, same message-size text, only the
        # "<X.YYY> us" timing varies and that is stripped below); one shared
        # refFile is sufficient.
        testDataFileName = "test_ember_networkIO_stripe_{0}".format(policy)

        reffile = "{0}/refFiles/test_ember_networkIO_stripe.out".format(test_path)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/testIO_stripe.py".format(test_path)

        # testIO_stripe.py reads the stripe policy from sys.argv[1].  SST
        # forwards tokens after --model-options="..." (or after a bare --)
        # into sys.argv, so the policy name is the only model option.
        otherargs = '--model-options="{0}"'.format(policy)

        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                     set_cwd=self.emberNetworkIO_Folder, mpi_out_files=mpioutfiles)

        # Strip the numeric "<X.YYY> us" suffix to make the diff content-only
        # (SimpleSSD enqueue rotation jitters the sub-microsecond timing).
        # Sort the result because under np>=2 the motif lines come from the
        # ember rank while "Simulation is complete" comes from the core rank
        # in non-deterministic order.
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
                "Ember NetworkIO stripe test {0} has a Non-Empty Error File {1}".format(testDataFileName, errfile)
            )

#####

    def Ember_stripe_balance_template(self, policy):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        self.emberNetworkIO_Folder = "{0}/ember_networkIO_folder".format(tmpdir)

        testDataFileName = "test_ember_networkIO_stripe_{0}_balance".format(policy)
        outfile = "{0}/{1}.out".format(outdir, testDataFileName)
        errfile = "{0}/{1}.err".format(outdir, testDataFileName)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testDataFileName)
        sdlfile = "{0}/../test/testIO_stripe.py".format(test_path)

        stats_csv_name = "okmar_stripe_stats_{0}.csv".format(policy)
        nid_sidecar_name = "okmar_stripe_io_nids.txt"

        otherargs = '--model-options="{0} {1} {2}"'.format(
            policy, STRIPE_COV_ITERATIONS, stats_csv_name)

        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                     set_cwd=self.emberNetworkIO_Folder, mpi_out_files=mpioutfiles)

        nid_path = "{0}/{1}".format(self.emberNetworkIO_Folder, nid_sidecar_name)
        self.assertTrue(os.path.isfile(nid_path),
                        "Stripe IO-NID sidecar not produced: {0}".format(nid_path))

        with open(nid_path) as f:
            io_nids = [int(x) for x in f.read().strip().split(",") if x]
        self.assertEqual(len(io_nids), 2,
                         "Expected 2 I/O NIDs in sidecar, got {0}".format(io_nids))

        # SST appends _<rank> before the extension when running multi-rank,
        # so glob to find either the single-rank file or all per-rank shards.
        stem, ext = os.path.splitext(stats_csv_name)
        csv_paths = sorted(glob.glob("{0}/{1}{2}".format(self.emberNetworkIO_Folder, stem, ext))
                           + glob.glob("{0}/{1}_*{2}".format(self.emberNetworkIO_Folder, stem, ext)))
        self.assertTrue(csv_paths,
                        "Stripe stats CSV not produced: {0}/{1}".format(self.emberNetworkIO_Folder, stats_csv_name))

        io_nic_names = {"nic{0}".format(n) for n in io_nids}
        per_nic_sum = {}
        for csv_path in csv_paths:
            with open(csv_path) as f:
                reader = csv.reader(f, skipinitialspace=True)
                header = next(reader)
                try:
                    name_idx = header.index("ComponentName")
                    stat_idx = header.index("StatisticName")
                    sum_idx = header.index("Sum.u64")
                except ValueError as exc:
                    self.fail("Unexpected CSV header {0}: {1}".format(header, exc))
                for row in reader:
                    if len(row) <= sum_idx:
                        continue
                    if row[stat_idx] != "rcvdByteCount":
                        continue
                    if row[name_idx] not in io_nic_names:
                        continue
                    per_nic_sum[row[name_idx]] = int(row[sum_idx])

        self.assertEqual(set(per_nic_sum.keys()), io_nic_names,
                         "Missing rcvdByteCount rows for I/O nics; got {0}, expected {1}".format(
                             set(per_nic_sum.keys()), io_nic_names))

        sums = list(per_nic_sum.values())
        mean = sum(sums) / len(sums)
        self.assertGreater(mean, 0, "Mean received bytes is zero: {0}".format(per_nic_sum))
        cov = statistics.pstdev(sums) / mean
        log_debug("Stripe balance policy={0} per-nic={1} CoV={2:.4f}".format(policy, per_nic_sum, cov))
        self.assertLessEqual(
            cov, STRIPE_COV_TOLERANCE,
            "Stripe policy '{0}' CoV {1:.4f} exceeds tolerance {2}: per-nic {3}".format(
                policy, cov, STRIPE_COV_TOLERANCE, per_nic_sum))

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO stripe balance test {0} has a Non-Empty Error File {1}".format(
                    testDataFileName, errfile)
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
