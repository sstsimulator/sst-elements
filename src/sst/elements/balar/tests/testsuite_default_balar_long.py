# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

from testBalar_testsuite_util import *

class testcase_balar_long(BalarTestCase):
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_backprop_2048(self):
        self.balar_vanadis_clang_template("rodinia-2.0-backprop-2048", 3600)
        
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_bfs_graph4096(self):
        self.balar_vanadis_clang_template("rodinia-2.0-bfs-graph4096", 4000)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_lud_256(self):
        self.balar_vanadis_clang_template("rodinia-2.0-lud-256", 7200)