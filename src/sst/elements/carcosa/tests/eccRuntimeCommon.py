import sst
try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

def build(guard_params, driver_params=None, name=""):
    suffix = "_" + name if name else ""
    state_key = "ecc_runtime_test" + suffix
    dp = {"state_key": state_key}
    if driver_params:
        dp.update(driver_params)
    driver = sst.Component("driver" + suffix, "carcosa.EccRuntimeTestDriver")
    driver.addParams(dp)
    guard = sst.Component("guard" + suffix, "carcosa.EccGuard")
    gp = {"state_key": state_key, "apply_on_responses_only": "true", "seed": 12345}
    gp.update(guard_params)
    guard.addParams(gp)
    sst.Link("cpu_guard" + suffix).connect((driver, "cpu_side", "1ns"),
                                  (guard, "highlink", "1ns"))
    sst.Link("guard_mem" + suffix).connect((guard, "lowlink", "1ns"),
                                  (driver, "mem_side", "1ns"))
