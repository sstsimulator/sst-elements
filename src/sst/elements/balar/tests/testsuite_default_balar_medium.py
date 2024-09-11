# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

from testBalar_testsuite_util import *

class testcase_balar_medium(BalarTestCase):
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_backprop_1024(self):
        self.balar_vanadis_clang_template("rodinia-2.0-backprop-1024", 2400)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_hotspot_30_6_40(self):
        self.balar_vanadis_clang_template("rodinia-2.0-hotspot-30-6-40", 1200)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_lud_64(self):
        self.balar_vanadis_clang_template("rodinia-2.0-lud-64", 1200)
        
    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_nw_128_10(self):
        self.balar_vanadis_clang_template("rodinia-2.0-nw-128-10", 1200)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_pathfinder_1000_20_5(self):
        self.balar_vanadis_clang_template("rodinia-2.0-pathfinder-1000-20-5", 1500)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_srad_v2_128x128(self):
        self.balar_vanadis_clang_template("rodinia-2.0-srad_v2-128x128", 40 * 60)