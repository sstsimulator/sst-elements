# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
# All rights reserved.

"""Compile the wire header without SST or GPGPU-Sim.

Run with python3 -m unittest test_balar_packet_wire from this directory.
CUDA_INCLUDE_PATH, or CUDA_INSTALL_PATH/include, enables the real-toolkit check.
The CUDA fixture always tests the enum-tag declaration that differs in C/C++.
"""

import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


SOURCE = r"""
#include "balar_packet_wire.h"
#ifdef __cplusplus
#include <type_traits>
using namespace SST::BalarComponent;
#define CHECK static_assert
#ifdef BALAR_PACKET_WIRE_HAS_CUDA_TYPES
static_assert(std::is_same<decltype(BalarCudaCallPacket_t{}.cudaDeviceGetAttribute.attr),
                           enum cudaDeviceAttr>::value, "Preserve the CUDA enum type");
#else
static_assert(std::is_same<decltype(BalarCudaCallPacket_t{}.cudaDeviceGetAttribute.attr),
                           cudaDeviceAttr>::value, "Preserve the peer's attribute type");
#endif
#else
#define CHECK _Static_assert
#endif
#if defined(EXPECT_CUDA) && !defined(BALAR_PACKET_WIRE_HAS_CUDA_TYPES)
#error CUDA headers were not selected
#endif
#if defined(EXPECT_FIRMWARE) && defined(BALAR_PACKET_WIRE_HAS_CUDA_TYPES)
#error Firmware types must take precedence over CUDA headers
#endif
CHECK(sizeof(BalarCudaCallPacket_t) == 536, "Request wire size changed");
CHECK(offsetof(BalarCudaCallPacket_t, cudaDeviceGetAttribute.attr) == 8,
      "Attribute wire offset changed");
CHECK(offsetof(BalarCudaCallPacket_t, cudaDeviceGetAttribute.device) == 12,
      "Device wire offset changed");
CHECK(offsetof(BalarCudaCallReturnPacket_t, fat_cubin_handle) == 16,
      "Return value wire offset changed");
int main(void) {
    BalarCudaCallPacket_t packet;
    packet.cuda_call_id = CUDA_DEVICE_GET_ATTRIBUTE;
#ifdef BALAR_PACKET_WIRE_HAS_CUDA_TYPES
    packet.cudaDeviceGetAttribute.attr = cudaDevAttrMaxThreadsPerBlock;
#else
    packet.cudaDeviceGetAttribute.attr = (cudaDeviceAttr)1;
#endif
    packet.cudaDeviceGetAttribute.device = 0;
    return packet.cudaDeviceGetAttribute.device;
}
"""

# CUDA deliberately has no C typedef for cudaDeviceAttr. Firmware does.
TYPE_FIXTURE = r"""
#include <stdint.h>
typedef int cudaError_t;
typedef void *cudaStream_t;
typedef void *cudaEvent_t;
ATTRIBUTE_DECLARATION
enum cudaMemcpyKind { cudaMemcpyHostToHost, cudaMemcpyHostToDevice,
                      cudaMemcpyDeviceToHost, cudaMemcpyDeviceToDevice,
                      cudaMemcpyDefault };
struct textureReference { uint8_t reserved[128]; };
struct cudaChannelFormatDesc { int x, y, z, w, f; };
struct cudaDeviceProp { uint8_t reserved[1024]; };
"""


class TestBalarPacketWire(unittest.TestCase):
    def compile_languages(self, include_dirs=(), defines=(), preinclude=None):
        header_dir = Path(__file__).resolve().parent.parent
        for language, compiler_var, default, standard in (
                ("c", "CC", "cc", "c11"),
                ("c++", "CXX", "c++", "c++11")):
            with self.subTest(language=language):
                command = shlex.split(os.environ.get(compiler_var, default))
                if not command or shutil.which(command[0]) is None:
                    self.skipTest("{} compiler not available".format(language))
                command += ["-std=" + standard, "-fsyntax-only", "-I", str(header_dir)]
                for directory in include_dirs:
                    command += ["-I", str(directory)]
                for define in defines:
                    command += ["-D" + define]
                if preinclude is not None:
                    command += ["-include", str(preinclude)]
                command += ["-x", language, "-"]
                result = subprocess.run(command, input=SOURCE, text=True,
                                        capture_output=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_default_include_path(self):
        # Exercises the standalone fallback when CUDA is not in system includes.
        self.compile_languages()

    def test_cuda_enum_headers(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / "builtin_types.h").write_text(TYPE_FIXTURE.replace(
                "ATTRIBUTE_DECLARATION", "enum cudaDeviceAttr { cudaDevAttrMaxThreadsPerBlock = 1 };"))
            (path / "driver_types.h").write_text("")
            self.compile_languages([path], ["EXPECT_CUDA"])

    def test_firmware_overrides_cuda_headers(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            for header in ("builtin_types.h", "driver_types.h"):
                (path / header).write_text('#error Firmware must not include host CUDA headers\n')
            firmware = path / "firmware_types.h"
            firmware.write_text("#define CUDA_RUNTIME_TYPES_FIRMWARE_H\n" + TYPE_FIXTURE.replace(
                "ATTRIBUTE_DECLARATION", "typedef int cudaDeviceAttr;"))
            self.compile_languages([path], ["EXPECT_FIRMWARE"], firmware)

    def test_installed_cuda_headers(self):
        include = os.environ.get("CUDA_INCLUDE_PATH")
        if not include:
            install = os.environ.get("CUDA_INSTALL_PATH") or os.environ.get("CUDA_PATH")
            if install:
                include = str(Path(install) / "include")
        if not include:
            self.skipTest("CUDA_INCLUDE_PATH or CUDA_INSTALL_PATH is not set")
        self.compile_languages([include], ["EXPECT_CUDA"])


if __name__ == "__main__":
    unittest.main()
