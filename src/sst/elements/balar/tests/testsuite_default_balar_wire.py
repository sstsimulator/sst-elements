# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
# All rights reserved.

from sst_unittest import SSTTestCase, setUpModule, tearDownModule
import test_balar_packet_wire as wire_tests


class testcase_balar_wire(wire_tests.TestBalarPacketWire, SSTTestCase):
    """Compile the shared header without requiring a Balar/GPGPU-Sim build."""
