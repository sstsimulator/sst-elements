# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import csv
import glob
import os


class testcase_EmberNetworkIO_Interference(SSTTestCase):

    def setUp(self):
        super(type(self), self).setUp()
        self._cleanupTestFiles()
        self._setupTestFiles()

    def tearDown(self):
        super(type(self), self).tearDown()

    def test_Ember_NetworkIO_interference_proxy(self):
        baseline = self._runMode("baseline", "okmar_interference_baseline.csv")
        contention = self._runMode("contention", "okmar_interference_contention.csv")

        self.assertGreater(baseline["count"], 0, "baseline produced no samples")
        self.assertGreater(contention["count"], 0, "contention produced no samples")

        mean_delta = (contention["mean_ns"] - baseline["mean_ns"]) / baseline["mean_ns"]
        max_delta = (contention["max_ns"] - baseline["max_ns"]) / baseline["max_ns"]

        report = (
            "[NetworkIO interference proxy] "
            "baseline=(n={bn}, mean={bm:.1f}ns, max={bx}ns)  "
            "contention=(n={cn}, mean={cm:.1f}ns, max={cx}ns)  "
            "delta(mean)={dm:+.2%}  delta(max)={dx:+.2%}"
        ).format(
            bn=baseline["count"], bm=baseline["mean_ns"], bx=baseline["max_ns"],
            cn=contention["count"], cm=contention["mean_ns"], cx=contention["max_ns"],
            dm=mean_delta, dx=max_delta,
        )
        log_testing_note(report)

        outdir = self.get_test_output_run_dir()
        with open("{0}/test_ember_networkIO_interference.summary".format(outdir), "w") as f:
            f.write(report + "\n")

    def _runMode(self, mode, csv_name):
        test_path = self.get_testsuite_dir()
        outdir = self.get_test_output_run_dir()
        tmpdir = self.get_test_output_tmp_dir()
        run_folder = "{0}/ember_networkIO_interference_{1}".format(tmpdir, mode)
        self._ensureFolder(run_folder)

        testcase = "test_ember_networkIO_interference_{0}".format(mode)
        outfile = "{0}/{1}.out".format(outdir, testcase)
        errfile = "{0}/{1}.err".format(outdir, testcase)
        mpioutfiles = "{0}/{1}.testfile".format(outdir, testcase)
        sdlfile = "{0}/../test/testIO_interference.py".format(test_path)

        self.run_sst(
            sdlfile, outfile, errfile,
            other_args='--model-options="{0} 200 {1}"'.format(mode, csv_name),
            set_cwd=run_folder,
            mpi_out_files=mpioutfiles,
        )

        stem, ext = os.path.splitext(csv_name)
        candidates = sorted(
            glob.glob("{0}/{1}{2}".format(run_folder, stem, ext))
            + glob.glob("{0}/{1}_*{2}".format(run_folder, stem, ext))
        )
        self.assertTrue(
            len(candidates) > 0,
            "No CSV produced for mode={0} (looked in {1})".format(mode, run_folder),
        )

        sum_ns = 0
        count = 0
        max_ns = 0
        for path in candidates:
            with open(path, newline="") as f:
                reader = csv.reader(f, skipinitialspace=True)
                for i, row in enumerate(reader):
                    if i == 0 or len(row) != 11:
                        continue
                    if row[1] != "networkIoWriteLatency_ns":
                        continue
                    cnt = int(row[8])
                    if cnt == 0:
                        continue
                    sum_ns += int(row[6])
                    count += cnt
                    max_ns = max(max_ns, int(row[10]))

        mean_ns = (sum_ns / count) if count else 0.0
        return {"count": count, "sum_ns": sum_ns, "mean_ns": mean_ns, "max_ns": max_ns}

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
