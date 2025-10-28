# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

from testBalar_testsuite_util import *

## TODO
## Commenting-out some of these tests temporarily
## Should re-enable once the basic vectorAdd test is integrated
##
class testcase_balar_simple(BalarTestCase):
    # @BalarTestCase.balar_basic_unittest
    # def test_balar_runvecadd_testcpu(self):
    #     self.balar_testcpu_template("vectorAdd")
    #
    # @BalarTestCase.balar_basic_unittest
    # def test_balar_runvecadd_vanadis(self):
    #     self.balar_vanadis_template("vanadisHandshake")
    #
    # @BalarTestCase.balar_basic_unittest
    # def test_balar_vanadis_clang_helloworld(self):
    #     self.balar_vanadis_clang_template("helloworld")

    @BalarTestCase.balar_basic_unittest
    # @unittest.skipIf(not testing_check_is_nightly(), "balar tests only run on Nightly builds.")
    def test_balar_vanadis_clang_vecadd(self):
        self.balar_vanadis_clang_template("vecadd", 60 * 30)
    #
    # @BalarTestCase.balar_gpuapp_unittest
    # def test_balar_vanadis_clang_rodinia_20_backprop_short(self):
    #     self.balar_vanadis_clang_template("rodinia-2.0-backprop-short", 60 * 30)
    #
    # @BalarTestCase.balar_gpuapp_unittest
    # def test_balar_vanadis_clang_rodinia_20_bfs_SampleGraph(self):
    #     self.balar_vanadis_clang_template("rodinia-2.0-bfs-SampleGraph")

