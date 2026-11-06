"""A FourStateAgent snapshot must exist regardless of watcher setup order."""
import sst

try:
    import sst.memHierarchy  # noqa: F401
except ModuleNotFoundError:
    pass

base = 0xBEEF0000


def build(prefix, watcher_first):
    driver = sst.Component(prefix + ".driver", "carcosa.HaliTestDriver")
    driver.addParams({"mode": "payloadless_getx", "base": base})

    def make_hali():
        hali = sst.Component(prefix + ".hali", "carcosa.Hali")
        hali.addParams({"intercept_ranges": "0x%x,4096" % base})
        agent = hali.setSubComponent("interceptionAgent", "carcosa.FourStateAgent")
        agent.addParams({"state_key": prefix, "regions": "action_queue:0x2000:64"})
        return hali

    def make_watcher():
        watcher = sst.Component(prefix + ".watcher", "carcosa.CriticalActionWatcher")
        watcher.addParams({"state_key": prefix})
        return watcher

    if watcher_first:
        watcher = make_watcher()
        hali = make_hali()
    else:
        hali = make_hali()
        watcher = make_watcher()

    sst.Link(prefix + ".cpu_hali").connect((driver, "cpu_side", "1ns"),
                                           (hali, "highlink", "1ns"))
    sst.Link(prefix + ".hali_watcher").connect((hali, "lowlink", "1ns"),
                                               (watcher, "highlink", "1ns"))
    sst.Link(prefix + ".watcher_mem").connect((watcher, "lowlink", "1ns"),
                                              (driver, "mem_side", "1ns"))


build("hali_first", watcher_first=False)
build("watcher_first", watcher_first=True)
sst.setProgramOption("stop-at", "1us")
