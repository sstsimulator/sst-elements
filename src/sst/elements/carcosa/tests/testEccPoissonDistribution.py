from eccRuntimeCommon import build
# N=5000, lambda=64*0.01=.64, P(any)=1-exp(-.64)=.473.
build({"fault_model": "poisson", "ecc_scheme": "none", "ber": 0.01,
       "test_total_min": 5000, "test_total_max": 5000,
       "test_escape_min": 2200, "test_escape_max": 2500},
      {"requests": 5000, "expect_abort": 0})
