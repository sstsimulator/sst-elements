# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

from testBalar_testsuite_util import *

## Balar integration tests
## Requires: CUDA_INSTALL_PATH, GPGPUSIM_ROOT, NVCC


class testcase_balar_smoke(SSTTestCase):
    """Tier-0: no GPGPU-Sim required."""

    def setUp(self):
        super().setUp()
        self._setupbalarVarEnv_smoke()

    def _setupbalarVarEnv_smoke(self):
        test_path = self.get_testsuite_dir()
        self.balarElementDir = os.path.abspath("{0}/../".format(test_path))
        self.testbalarDir = "{0}/test_balar_smoke".format(self.get_test_output_tmp_dir())
        if os.path.isdir(self.testbalarDir):
            shutil.rmtree(self.testbalarDir, True)
        os.makedirs(self.testbalarDir)
        for f in ("balarBlock.py", "utils.py", "gpu-v100-mem.cfg"):
            os_symlink_file(test_path, self.testbalarDir, f)

    def _sst_info_text(self):
        import subprocess
        rtn = subprocess.run(["sst-info", "balar"], capture_output=True, text=True, timeout=30)
        return rtn.stdout + rtn.stderr, rtn.returncode

    def test_balar_sst_info_registers_components(self):
        """sst-info balar lists balarMMIO when the element is built."""
        text, rc = self._sst_info_text()
        if "UNABLE TO PROCESS LIBRARY" in text.upper() or rc != 0:
            self.skipTest("balar element not built (configure without --with-gpgpusim)")
        self.assertIn("balarMMIO", text, "sst-info balar should list balar.balarMMIO")

    def test_balar_doorbell_testcpu_registered(self):
        """sst-info balar lists DoorbellTestCPU when the element is built."""
        text, rc = self._sst_info_text()
        if "UNABLE TO PROCESS LIBRARY" in text.upper() or rc != 0:
            self.skipTest("balar element not built (configure without --with-gpgpusim)")
        self.assertIn("DoorbellTestCPU", text, "sst-info balar should list balar.DoorbellTestCPU")


class testcase_balar_simple(BalarTestCase):
    @BalarTestCase.balar_basic_unittest
    def test_balar_runvecadd_testcpu(self):
        self.balar_testcpu_template("vectorAdd", testtimeout=60 * 20)

    @BalarTestCase.balar_basic_unittest
    def test_balar_contract_doorbell(self):
        self.balar_contract_testcpu_template(
            "doorbell", "testBalar-doorbell.py", "doorbell_malloc.trace", testtimeout=60 * 10)

    @BalarTestCase.balar_basic_unittest
    def test_balar_contract_malloc_free(self):
        self.balar_contract_testcpu_template(
            "malloc_free", "testBalar-malloc-free.py", "malloc_free.trace", testtimeout=60 * 15)

    @BalarTestCase.balar_basic_unittest
    def test_balar_contract_wide_packet(self):
        self.balar_contract_testcpu_template(
            "wide_packet", "testBalar-wide-packet.py", "wide_memcpy_d2h.trace", testtimeout=60 * 20)

    @BalarTestCase.balar_basic_unittest
    def test_doorbellcpu_doorbell(self):
        self.doorbell_contract_testcpu_template(
            "doorbellcpu_doorbell", "testDoorbellCPU-doorbell.py", None, testtimeout=60 * 10)

    @BalarTestCase.balar_basic_unittest
    def test_doorbellcpu_malloc_free(self):
        self.doorbell_contract_testcpu_template(
            "doorbellcpu_malloc_free", "testDoorbellCPU-malloc-free.py", "malloc_free.trace", testtimeout=60 * 15)

    @BalarTestCase.balar_basic_unittest
    def test_doorbellcpu_wide_packet(self):
        self.doorbell_contract_testcpu_template(
            "doorbellcpu_wide_packet", "testDoorbellCPU-wide-packet.py", "wide_memcpy_d2h.trace",
            testtimeout=60 * 20, min_flush_count=64)

    @unittest.skipIf(
        os.getenv("LLVM_INSTALL_PATH") is None or os.getenv("RISCV_TOOLCHAIN_INSTALL_PATH") is None,
        "Vanadis clang vecadd requires LLVM_INSTALL_PATH and RISCV_TOOLCHAIN_INSTALL_PATH")
    @BalarTestCase.balar_basic_unittest
    def test_balar_vanadis_clang_vecadd(self):
        self.balar_vanadis_clang_template("vecadd", 60 * 30)
