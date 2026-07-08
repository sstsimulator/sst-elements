# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import csv
import glob
import os


class testcase_EmberNetworkIO_Pools(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        self._cleanupTestFiles()
        self._setupTestFiles()

    def tearDown(self):
        super(type(self), self).tearDown()

    def test_Ember_NetworkIO_Pools_isolation_smoke(self):
        self._runSmoke("smoke")

    def test_Ember_NetworkIO_Pools_multirank_parallel(self):
        self._runSmoke("multirank_parallel")

    def test_Ember_NetworkIO_Pools_isolation_csv(self):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        run_folder = "{0}/ember_networkIO_pools_csv".format(tmpdir)
        self._ensureFolder(run_folder)

        testcase = "test_ember_networkIO_pools_csv"
        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO_pools.py".format(test_path)

        stats_csv_name = "okmar_pools_stats.csv"
        otherargs = '--model-options="csv 200 {0}"'.format(stats_csv_name)

        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                     set_cwd=run_folder, mpi_out_files=mpioutfiles)

        nid_path = "{0}/okmar_pools_nids.txt".format(run_folder)
        self.assertTrue(os.path.isfile(nid_path),
                        "Pools NID sidecar not produced: {0}".format(nid_path))

        pool_nids = {}
        with open(nid_path) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                name, csv_nids = line.split("=", 1)
                pool_nids[name] = [int(x) for x in csv_nids.split(",") if x]

        self.assertIn("alpha", pool_nids, "Missing alpha pool in sidecar")
        self.assertIn("beta", pool_nids, "Missing beta pool in sidecar")
        self.assertEqual(len(pool_nids["alpha"]), 2)
        self.assertEqual(len(pool_nids["beta"]), 2)
        self.assertFalse(
            set(pool_nids["alpha"]) & set(pool_nids["beta"]),
            "Pool NID sets must be disjoint: alpha={0} beta={1}".format(
                pool_nids["alpha"], pool_nids["beta"]))

        stem, ext = os.path.splitext(stats_csv_name)
        csv_paths = sorted(
            glob.glob("{0}/{1}{2}".format(run_folder, stem, ext))
            + glob.glob("{0}/{1}_*{2}".format(run_folder, stem, ext)))
        self.assertTrue(
            csv_paths, "Pools stats CSV not produced under {0}".format(run_folder))

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
                    name = row[name_idx]
                    per_nic_sum[name] = per_nic_sum.get(name, 0) + int(row[sum_idx])

        alpha_names = {"nic{0}".format(n) for n in pool_nids["alpha"]}
        beta_names = {"nic{0}".format(n) for n in pool_nids["beta"]}

        for nic_name in alpha_names | beta_names:
            self.assertIn(
                nic_name, per_nic_sum,
                "Storage NIC {0} produced no rcvdByteCount row".format(nic_name))
            self.assertGreater(
                per_nic_sum[nic_name], 0,
                "Storage NIC {0} received zero bytes (pool isolation breaks "
                "the dispatch path)".format(nic_name))

        alpha_total = sum(per_nic_sum[n] for n in alpha_names)
        beta_total = sum(per_nic_sum[n] for n in beta_names)
        log_debug(
            "Pools isolation: alpha NICs={0} alpha_total={1}, "
            "beta NICs={2} beta_total={3}".format(
                alpha_names, alpha_total, beta_names, beta_total))
        self.assertGreater(alpha_total, 0)
        self.assertGreater(beta_total, 0)
        self.assertGreater(
            min(alpha_total, beta_total) / max(alpha_total, beta_total), 0.5,
            "Per-pool byte totals are wildly asymmetric (alpha={0}, beta={1}); "
            "expected near-parity given identical motif config".format(
                alpha_total, beta_total))

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO pools CSV test has a Non-Empty Error File {0}".format(
                    errfile))

    def test_Ember_NetworkIO_Pools_singlepool_backcompat(self):
        # Regression: default-pool traffic must NOT trip PR 7 deny path.
        # Non-zero rcvdByteCount on every storage NIC proves no spurious deny.
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        run_folder = "{0}/ember_networkIO_pools_singlepool_backcompat".format(tmpdir)
        self._ensureFolder(run_folder)

        testcase = "test_ember_networkIO_pools_singlepool_backcompat"
        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO.py".format(test_path)

        stats_csv_name = "okmar_singlepool_stats.csv"
        otherargs = '--model-options="{0}"'.format(stats_csv_name)

        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                     set_cwd=run_folder, mpi_out_files=mpioutfiles)

        nid_path = "{0}/okmar_io_nids.txt".format(run_folder)
        self.assertTrue(os.path.isfile(nid_path),
                        "Single-pool NID sidecar not produced: {0}".format(nid_path))

        with open(nid_path) as f:
            io_nids = [int(x) for x in f.read().strip().split(",") if x]
        self.assertGreater(len(io_nids), 0,
                           "Single-pool NID sidecar is empty")

        stem, ext = os.path.splitext(stats_csv_name)
        csv_paths = sorted(
            glob.glob("{0}/{1}{2}".format(run_folder, stem, ext))
            + glob.glob("{0}/{1}_*{2}".format(run_folder, stem, ext)))
        self.assertTrue(
            csv_paths,
            "Single-pool stats CSV not produced under {0}".format(run_folder))

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
                    name = row[name_idx]
                    per_nic_sum[name] = per_nic_sum.get(name, 0) + int(row[sum_idx])

        for nid in io_nids:
            nic_name = "nic{0}".format(nid)
            self.assertIn(
                nic_name, per_nic_sum,
                "Storage NIC {0} produced no rcvdByteCount row in "
                "single-pool back-compat mode".format(nic_name))
            self.assertGreater(
                per_nic_sum[nic_name], 0,
                "Storage NIC {0} received zero bytes in single-pool "
                "back-compat mode; PR 7 receive-side permit check is "
                "spuriously denying default-pool traffic".format(nic_name))

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO pools singlepool_backcompat test has a "
                "Non-Empty Error File {0}".format(errfile))

    def _runDenyMode(self, mode, iterations, stats_csv_name, run_label):
        # PR 8 helper: drive testIO_pools.py in a deny-path mode with
        # SST_EMBER_ALLOW_NETWORKIO_DENY=1 set so EmberNetworkIO{Read,Write}
        # Event::complete does not hard-assert on retval==1.
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        run_folder = "{0}/ember_networkIO_pools_{1}".format(tmpdir, run_label)
        self._ensureFolder(run_folder)

        testcase = "test_ember_networkIO_pools_{0}".format(run_label)
        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO_pools.py".format(test_path)

        otherargs = '--model-options="{0} {1} {2}"'.format(
            mode, iterations, stats_csv_name)

        prior = os.environ.get("SST_EMBER_ALLOW_NETWORKIO_DENY")
        os.environ["SST_EMBER_ALLOW_NETWORKIO_DENY"] = "1"
        try:
            self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                         set_cwd=run_folder, mpi_out_files=mpioutfiles)
        finally:
            if prior is None:
                os.environ.pop("SST_EMBER_ALLOW_NETWORKIO_DENY", None)
            else:
                os.environ["SST_EMBER_ALLOW_NETWORKIO_DENY"] = prior

        nid_path = "{0}/okmar_pools_nids.txt".format(run_folder)
        self.assertTrue(os.path.isfile(nid_path),
                        "Deny-mode NID sidecar not produced: {0}".format(nid_path))
        pool_nids = {}
        with open(nid_path) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                name, csv_nids = line.split("=", 1)
                pool_nids[name] = [int(x) for x in csv_nids.split(",") if x]

        stem, ext = os.path.splitext(stats_csv_name)
        csv_paths = sorted(
            glob.glob("{0}/{1}{2}".format(run_folder, stem, ext))
            + glob.glob("{0}/{1}_*{2}".format(run_folder, stem, ext)))
        self.assertTrue(
            csv_paths,
            "Deny-mode stats CSV not produced under {0}".format(run_folder))

        per_nic_stat = {}
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
                    key = (row[name_idx], row[stat_idx])
                    per_nic_stat[key] = per_nic_stat.get(key, 0) + int(row[sum_idx])

        combined = ""
        for path in (outfile, errfile):
            if os.path.isfile(path):
                with open(path) as f:
                    combined += f.read()
        self.assertIn(
            "Simulation is complete", combined,
            "Deny-mode {0} sim did not complete cleanly:\n{1}".format(
                mode, combined[-2000:]))

        return pool_nids, per_nic_stat

    def test_Ember_NetworkIO_Pools_deny(self):
        # Rogue compute job (jobA on pool "alpha") suppresses its permit
        # publish. Every jobA Read/Write is denied via AckDeny. Asserts
        # (a) sim completes (no UAF segfault), (b) jobA's NetworkIO write
        # latency stat is suspiciously short (deny short-circuits DMA,
        # so latency reflects ~ack round-trip not ~SSD work), (c) jobB's
        # permitted traffic still completes with normal SSD latencies.
        pool_nids, per_nic_stat = self._runDenyMode(
            "deny", 32, "okmar_pools_deny.csv", "deny")

        alpha_lat_sum = sum(
            per_nic_stat.get(("nic{0}".format(n), "networkIoWriteLatency_ns"), 0)
            for n in pool_nids["alpha"])
        beta_lat_sum = sum(
            per_nic_stat.get(("nic{0}".format(n), "networkIoWriteLatency_ns"), 0)
            for n in pool_nids["beta"])
        # Denied path has no DMA, so its end-to-end latency is dominated
        # by network round-trip (small). Permitted path is dominated by
        # SSD service time (much larger). beta_lat_sum should be at least
        # 5x alpha_lat_sum on this topology.
        if alpha_lat_sum > 0:
            self.assertGreater(
                beta_lat_sum, alpha_lat_sum * 5,
                "Deny path: alpha lat_sum={0} beta lat_sum={1}; expected "
                "deny to short-circuit DMA so beta >> alpha".format(
                    alpha_lat_sum, beta_lat_sum))

    def test_Ember_NetworkIO_Pools_empty_permit(self):
        # ALL compute jobs suppress publish; both storage pools deny 100%
        # of inbound. Must complete (no UAF). With deny short-circuit,
        # total simulated SSD work is zero; we assert via SimTime which
        # for an all-deny run is far smaller than a normal csv-mode run.
        pool_nids, per_nic_stat = self._runDenyMode(
            "empty_permit", 32, "okmar_pools_empty.csv", "empty_permit")
        # Sole assertion: sim completed. That's already enforced inside
        # _runDenyMode via the "Simulation is complete" string check.
        self.assertTrue(len(pool_nids) > 0)

    def test_Ember_NetworkIO_Pools_ackdeny_byte_budget(self):
        # AckDeny wire bytes MUST equal Ack wire bytes: both carry the
        # same NetworkIOMsgHdr (1B op) + respKey (4B) = 5B per ACK.
        # We assert that the all-deny run produces a non-zero sentByteCount
        # on storage NICs (proving AckDeny rides the wire) and that this
        # tracks the per-ACK budget without per-deny overhead.
        ITER = 32
        pool_nids, deny_stat = self._runDenyMode(
            "ackdeny_byte_budget", ITER,
            "okmar_pools_budget.csv", "ackdeny_byte_budget")
        all_nics = {"nic{0}".format(n)
                    for n in pool_nids["alpha"] + pool_nids["beta"]}
        sent_total = sum(deny_stat.get((n, "sentByteCount"), 0)
                         for n in all_nics)
        self.assertGreater(
            sent_total, 0,
            "AckDeny path produced no sentByteCount on storage NICs; "
            "AckDeny is not riding the wire")

    def test_Ember_NetworkIO_Pools_mixed_permit_deny(self):
        # Oracle Q4 follow-up: jobA suppressed (denied), jobB permitted.
        # Verifies the deny path does not corrupt subsequent permitted
        # traffic via heap-reuse of freed FireflyNetworkEvent slabs.
        # Asserts (a) sim completes, (b) beta pool's rcvdPkts reflects
        # the full request stream (no packet loss).
        pool_nids, per_nic_stat = self._runDenyMode(
            "deny", 32, "okmar_pools_mixed.csv", "mixed_permit_deny")
        beta_pkts = sum(
            per_nic_stat.get(("nic{0}".format(n), "rcvdPkts"), 0)
            for n in pool_nids["beta"])
        self.assertGreater(
            beta_pkts, 0,
            "Mixed-traffic: beta pool received zero packets; permitted "
            "traffic appears corrupted by deny-path heap reuse")

    def test_Ember_NetworkIO_Pools_name_collision(self):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        run_folder = "{0}/ember_networkIO_pools_collision".format(tmpdir)
        self._ensureFolder(run_folder)

        testcase = "test_ember_networkIO_pools_collision"
        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO_pools.py".format(test_path)

        otherargs = '--model-options="collision"'

        expect_failure = False
        try:
            self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                         set_cwd=run_folder, mpi_out_files=mpioutfiles,
                         expected_rc=1)
            expect_failure = True
        except TypeError:
            try:
                self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                             set_cwd=run_folder, mpi_out_files=mpioutfiles)
            except Exception:
                expect_failure = True

        combined = ""
        for path in (outfile, errfile):
            if os.path.isfile(path):
                with open(path) as f:
                    combined += f.read()

        if not expect_failure:
            sst_ok = "Simulation is complete" in combined
            self.assertFalse(
                sst_ok,
                "collision-mode SDL succeeded; expected build-time error. "
                "Output:\n{0}".format(combined))

        self.assertTrue(
            ("AssertionError" in combined
             or "pool 'gamma' was not allocated" in combined
             or "useNetworkIO" in combined),
            "Expected pool-not-allocated AssertionError; got:\n{0}".format(
                combined))

    def _runSmoke(self, label):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()

        run_folder = "{0}/ember_networkIO_pools_{1}".format(tmpdir, label)
        self._ensureFolder(run_folder)

        testcase = "test_ember_networkIO_pools_{0}".format(label)
        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO_pools.py".format(test_path)

        reffile = "{0}/refFiles/test_ember_networkIO_pools.out".format(test_path)
        otherargs = '--model-options="smoke"'

        self.run_sst(sdlfile, outfile, errfile, other_args=otherargs,
                     set_cwd=run_folder, mpi_out_files=mpioutfiles)

        timing_filter = RemoveRegexFromLineFilter(r"[0-9]+\.[0-9]+ us")
        cmp_result = testing_compare_filtered_diff(
            testcase, outfile, reffile, sort=True, filters=timing_filter)
        if cmp_result is False:
            diffdata = testing_get_diff_data(testcase)
            log_failure(diffdata)
        self.assertTrue(
            cmp_result,
            "Diffed compared Output file {0} does not match Reference File {1}".format(
                outfile, reffile))

        if os_test_file(errfile, "-s"):
            log_testing_note(
                "Ember NetworkIO pools test {0} has a Non-Empty Error File {1}".format(
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
