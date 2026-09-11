from eccRuntimeCommon import build
# Roughly 20us of serialized traffic at 5000 births/ms => mean near 100.
build({"fault_model": "resident", "ecc_scheme": "none",
       "resident_addr_start": 0x4000, "resident_addr_len": 4096,
       "resident_fault_rate_per_ms": 5000,
       "resident_permanent_fraction": 1, "resident_mode": "cell",
       "test_total_min": 5000, "test_total_max": 5000,
       "test_resident_born_min": 70, "test_resident_born_max": 130},
      {"requests": 5000, "expect_abort": 0})
