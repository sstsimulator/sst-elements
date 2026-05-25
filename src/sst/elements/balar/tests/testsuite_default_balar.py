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

    def test_balar_sst_info_registers_components(self):
        """sst-info balar lists balarMMIO when the element is built."""
        outdir = self.get_test_output_run_dir()
        outfile = "{0}/test_balar_smoke_info.out".format(outdir)
        errfile = "{0}/test_balar_smoke_info.err".format(outdir)
        cmd = "sst-info balar > '{0}' 2> '{1}'".format(outfile, errfile)
        rtn = os_command(cmd, set_cwd=self.testbalarDir).run()
        self.assertEqual(rtn.result(), 0, "sst-info balar failed: {0}".format(rtn.output()))
        with open(outfile, "r", errors="replace") as fh:
            text = fh.read()
        if "balarMMIO" in text:
            return
        if "not found" in text.lower() or "unable to find" in text.lower():
            self.skipTest("balar element not built (configure without --with-gpgpusim)")
        self.assertIn("balarMMIO", text, "sst-info balar should list balar.balarMMIO")

    def test_balar_python_topology_import(self):
        """Import balarBlock + balar_test_topology without CUDA."""
        import sys
        test_path = self.get_testsuite_dir()
        if test_path not in sys.path:
            sys.path.insert(0, test_path)
        import balarBlock  # noqa: F401
        import balar_test_topology  # noqa: F401


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
    def test_balar_contract_coherent_bus(self):
        self.balar_contract_testcpu_template(
            "coherent_bus", "testBalar-coherent-bus.py", "malloc_free.trace", testtimeout=60 * 15)

    @BalarTestCase.balar_basic_unittest
    def test_balar_contract_wide_packet(self):
        self.balar_contract_testcpu_template(
            "wide_packet", "testBalar-wide-packet.py", "wide_memcpy_d2h.trace", testtimeout=60 * 20)

    @unittest.skipIf(
        os.getenv("LLVM_INSTALL_PATH") is None or os.getenv("RISCV_TOOLCHAIN_INSTALL_PATH") is None,
        "Vanadis clang vecadd requires LLVM_INSTALL_PATH and RISCV_TOOLCHAIN_INSTALL_PATH")
    @BalarTestCase.balar_basic_unittest
    def test_balar_vanadis_clang_vecadd(self):
        self.balar_vanadis_clang_template("vecadd", 60 * 30)
