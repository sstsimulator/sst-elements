import sst
try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

def build(mode):
    base = 0xBEEF0000
    driver = sst.Component("driver", "carcosa.HaliTestDriver")
    driver.addParams({"mode": mode, "base": base})
    hali = sst.Component("hali", "carcosa.Hali")
    hali.addParams({"intercept_ranges": "0x%x,4096" % base})
    agent = hali.setSubComponent("interceptionAgent", "carcosa.HaliTestAgent")
    agent.addParams({"mode": mode})
    sst.Link("cpu_hali").connect((driver, "cpu_side", "1ns"),
                                 (hali, "highlink", "1ns"))
    sst.Link("hali_mem").connect((hali, "lowlink", "1ns"),
                                 (driver, "mem_side", "1ns"))
