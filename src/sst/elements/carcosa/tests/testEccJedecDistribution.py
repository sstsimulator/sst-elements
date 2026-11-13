from eccRuntimeCommon import build
# N=5000, explicit event probability .2 => mean 1000, sigma ~28.
build({"fault_model": "jedec_mix", "ecc_scheme": "none", "ber": 0.9,
       "fault_event_rate": 0.2,
       "test_total_min": 5000, "test_total_max": 5000,
       "test_escape_min": 900, "test_escape_max": 1100},
      {"requests": 5000, "expect_abort": 0})
