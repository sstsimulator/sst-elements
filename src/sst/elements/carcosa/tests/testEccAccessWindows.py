"""Address filters preserve address spaces and count excluded reads as clean."""
from eccRuntimeCommon import build


# A virtual address must not move a read out of its physical injection window.
# The second read is outside the window and must remain in the outcome totals.
build({"ecc_scheme": "none", "ber": 1,
       "inject_addr_start": 0x4000, "inject_addr_len": 8,
       "escape_latency_ps": 1000,
       "test_total_min": 2, "test_total_max": 2,
       "test_clean_min": 1, "test_clean_max": 1,
       "test_escape_min": 1, "test_escape_max": 1},
      {"requests": 2, "request_addresses": "0x4000,0x4040",
       "virtual_offset": 0x10000,
       "expect_mutated_sequence": "1,0", "expect_escapes": 1},
      name="physical_window")

# Conversely, a virtual address inside the window cannot admit physical data
# outside it. This also exercises the all-filtered outcome totals.
build({"ecc_scheme": "none", "ber": 1,
       "inject_addr_start": 0x14000, "inject_addr_len": 8,
       "test_total_min": 1, "test_total_max": 1,
       "test_clean_min": 1, "test_clean_max": 1,
       "test_escape_min": 0, "test_escape_max": 0},
      {"requests": 1, "request_addresses": "0x4000",
       "virtual_offset": 0x10000,
       "expect_mutated": 0, "expect_escapes": 0},
      name="virtual_alias")

# Published regions use virtual addresses. Only the first read belongs to the
# region; the excluded read still contributes a clean kernel/region outcome.
build({"ecc_scheme": "none", "ber": 1,
       "addr_filter_region": "weights",
       "test_total_min": 2, "test_total_max": 2,
       "test_clean_min": 1, "test_clean_max": 1,
       "test_escape_min": 1, "test_escape_max": 1},
      {"requests": 2, "request_addresses": "0x4000,0x4040",
       "virtual_offset": 0x10000,
       "region_name": "weights", "region_base": 0x14000, "region_size": 8,
       "expect_mutated_sequence": "1,0", "expect_escapes": 1},
      name="named_region")
