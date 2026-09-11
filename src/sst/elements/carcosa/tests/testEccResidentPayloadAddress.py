"""A resident cacheline fault has the same bytes for every request offset."""
from eccRuntimeCommon import build

for name, virtual_offset in (("physical", 0), ("virtual", 0x1000)):
    build({"fault_model": "resident", "ecc_scheme": "none",
           "resident_addr_start": 0x4000 + virtual_offset,
           "resident_addr_len": 128, "resident_faults_at_start": 64,
           "resident_mode": "cell", "resident_permanent_fraction": 1,
           "test_total_min": 3, "test_total_max": 3,
           "test_escape_min": 3, "test_escape_max": 3},
          {"requests": 3, "payload_size": 64,
           "request_addresses": "0x4000,0x4010,0x4008",
           "virtual_offset": virtual_offset, "expect_same_payload": True,
           "expect_mutated": 3, "expect_abort": 0, "expect_escapes": 3},
          name=name)
