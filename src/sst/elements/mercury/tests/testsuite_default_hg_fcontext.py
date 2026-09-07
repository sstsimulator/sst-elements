# -*- coding: utf-8 -*-

import platform
from pathlib import Path

from sst_unittest import *
from sst_unittest_support import *


class testcase_hg_fcontext(SSTTestCase):

    @unittest.skipUnless(
        platform.machine().lower() in ("arm64", "aarch64"),
        "AAPCS64 register test requires an ARM64 host",
    )
    @unittest.skipIf(
        testing_check_get_num_threads() > 1,
        "fcontext register test requires one SST thread",
    )
    @unittest.skipIf(
        testing_check_get_num_ranks() > 1,
        "fcontext register test requires one SST rank",
    )
    def test_arm64_fp_register_preservation(self):
        test_dir = Path(self.get_testsuite_dir())
        output_dir = Path(self.get_test_output_run_dir())
        output = output_dir / "fcontext_fp_preservation.out"
        error = output_dir / "fcontext_fp_preservation.err"

        self.run_sst(
            str(test_dir / "fcontext_fp_preservation.py"),
            str(output),
            str(error),
            set_cwd=str(test_dir),
            timeout_sec=10,
        )

        text = output.read_text(encoding="utf-8")
        self.assertEqual(1, text.count("Simulation is complete"))
        for rank in range(2):
            self.assertEqual(
                1,
                text.count(f"Mercury fcontext FP preservation rank {rank} PASS"),
            )
