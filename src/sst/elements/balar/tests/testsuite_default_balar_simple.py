# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

from testBalar_testsuite_util import *

class testcase_balar_simple(BalarTestCase):
    @BalarTestCase.balar_basic_unittest
    def test_balar_runvecadd_testcpu(self):
        self.balar_testcpu_template("vectorAdd")

    @BalarTestCase.balar_basic_unittest
    def test_balar_runvecadd_vanadis(self):
        self.balar_vanadis_template("vanadisHandshake")

    @BalarTestCase.balar_basic_unittest
    def test_balar_vanadis_clang_helloworld(self):
        self.balar_vanadis_clang_template("helloworld")
    
    @BalarTestCase.balar_basic_unittest
    def test_balar_vanadis_clang_vecadd(self):
        self.balar_vanadis_clang_template("vecadd")
        
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_backprop_short(self):
        self.balar_vanadis_clang_template("rodinia-2.0-backprop-short", 60 * 30)
    
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_bfs_SampleGraph(self):
        self.balar_vanadis_clang_template("rodinia-2.0-bfs-SampleGraph")