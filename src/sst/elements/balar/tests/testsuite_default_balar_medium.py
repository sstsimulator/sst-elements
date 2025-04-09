# -*- coding: utf-8 -*-

from sst_unittest import *
from sst_unittest_support import *

import os
import shutil

from testBalar_testsuite_util import *

class testcase_balar_medium(BalarTestCase):
    @BalarTestCase.balar_basic_unittest
    def test_balar_vanadis_clang_simpleStreams(self):
        self.balar_vanadis_clang_template("simpleStreams", 60 * 60)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_backprop_1024(self):
        self.balar_vanadis_clang_template("rodinia-2.0-backprop-1024", 4800)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_hotspot_30_6_40(self):
        self.balar_vanadis_clang_template("rodinia-2.0-hotspot-30-6-40", 2400)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_lud_64(self):
        self.balar_vanadis_clang_template("rodinia-2.0-lud-64", 2400)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_nw_128_10(self):
        self.balar_vanadis_clang_template("rodinia-2.0-nw-128-10", 2400)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_nn_4_3_30_90(self):
        self.balar_vanadis_clang_template("rodinia-2.0-nn-4-3-30-90", 2400)

    @BalarTestCase.balar_gpuapp_unittest
    def test_balar_vanadis_clang_rodinia_20_pathfinder_1000_20_5(self):
        self.balar_vanadis_clang_template("rodinia-2.0-pathfinder-1000-20-5", 3000)
