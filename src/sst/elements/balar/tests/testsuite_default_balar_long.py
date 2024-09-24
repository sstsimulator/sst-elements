# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

from testBalar_testsuite_util import *

class testcase_balar_long(BalarTestCase):
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_backprop_2048(self):
        self.balar_vanadis_clang_template("rodinia-2.0-backprop-2048", 60 * 120)
        
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_bfs_graph4096(self):
        self.balar_vanadis_clang_template("rodinia-2.0-bfs-graph4096", 60 * 120)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_lud_256(self):
        self.balar_vanadis_clang_template("rodinia-2.0-lud-256", 60 * 240)
    
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_srad_v2_128x128(self):
        self.balar_vanadis_clang_template("rodinia-2.0-srad_v2-128x128", 80 * 60)
        
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_streamcluster_3_6_16_1024_1024_100_none_1(self):
        self.balar_vanadis_clang_template("rodinia-2.0-streamcluster-3_6_16_1024_1024_100_none_1", 60 * 60)