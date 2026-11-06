"""FourStateAgent must consume posted status writes without sending an ack."""
import sst

try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

base = 0xBEEF0000
driver = sst.Component("driver", "carcosa.HaliTestDriver")
driver.addParams({"mode": "posted_write", "base": base})
hali = sst.Component("hali", "carcosa.Hali")
hali.addParams({"intercept_ranges": "0x%x,4096" % base})
agent = hali.setSubComponent("interceptionAgent", "carcosa.FourStateAgent")
# HaliTestDriver sends a posted status write followed by a command read and
# requires its only response to be a read of 0x12345678. No partner releases
# another command, so the initial command remains available for that read.
agent.addParams({"state_key": "fourstate_posted", "initial_command": 0x12345678})
sst.Link("cpu_hali").connect((driver, "cpu_side", "1ns"),
                             (hali, "highlink", "1ns"))
sst.Link("hali_mem").connect((hali, "lowlink", "1ns"),
                             (driver, "mem_side", "1ns"))
sst.setProgramOption("stop-at", "1us")
