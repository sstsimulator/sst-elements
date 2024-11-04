import sst
import os
import sys

sst.setProgramOption("timebase", "1ps")

app = os.getenv("ARIEL_EXE")
if app == None or not os.path.exists(app):
        sys.exit(os.EX_CONFIG)

allowed_args = ["redirect_in", "redirect_out", "redirect_err",
                "append_redirect_out", "append_redirect_err"]

for arg in sys.argv[1:]:
    if arg not in allowed_args:
        print("ERROR: {} Recieved unknown argument", sys.argv[0])
        sys.exit(os.EX_CONFIG)

ariel_params = {
        "verbose" : "0",
        "maxcorequeue" : "256",
        "maxissuepercycle" : "2",
        "pipetimeout" : "0",
        "executable" : app,
        "arielmode" : "1",
        "launchparamcount" : 1,
        "launchparam0" : "-ifeellucky",
}

if "redirect_in" in sys.argv:
    ariel_params["appstdin"] = "input.txt"

if "redirect_out" in sys.argv:
    ariel_params["appstdout"] = "app_stdout.txt"

if "redirect_err" in sys.argv:
    ariel_params["appstderr"] = "app_stderr.txt"

if "append_redirect_out" in sys.argv:
    f = open("app_stdout.txt", "w")
    f.write("We will append to this file.\n")
    f.close()
    ariel_params["appstdout"] = "app_stdout.txt"
    ariel_params["appstdoutappend"] = 1

if "append_redirect_err" in sys.argv:
    f = open("app_stderr.txt", "w")
    f.write("We will append to this file.\n")
    f.close()
    ariel_params["appstderr"] = "app_stderr.txt"
    ariel_params["appstderrappend"] = 1

ariel = sst.Component("a0", "ariel.ariel")
ariel.addParams(ariel_params)

memmgr = ariel.setSubComponent("memmgr", "ariel.MemoryManagerSimple")

l1cache = sst.Component("l1cache", "memHierarchy.Cache")
l1cache.addParams({
        "cache_frequency" : "2 Ghz",
        "cache_size" : "64 KB",
        "coherence_protocol" : "MSI",
        "replacement_policy" : "lru",
        "associativity" : "8",
        "access_latency_cycles" : "1",
        "cache_line_size" : "64",
        "L1" : "1",
        "debug" : "0",
})

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({
        "clock" : "1GHz",
        "addr_range_start" : 0,
})

memory = memctrl.setSubComponent("backend", "memHierarchy.simpleMem")
memory.addParams({
        "access_time" : "10ns",
        "mem_size" : "2048MiB",
})

cpu_cache_link = sst.Link("cpu_cache_link")
cpu_cache_link.connect( (ariel, "cache_link_0", "50ps"), (l1cache, "high_network_0", "50ps") )

memory_link = sst.Link("mem_bus_link")
memory_link.connect( (l1cache, "low_network_0", "50ps"), (memctrl, "direct_link", "50ps") )


# Set the Statistic Load Level; Statistics with Enable Levels (set in
# elementInfoStatistic) lower or equal to the load can be enabled (default = 0)
sst.setStatisticLoadLevel(5)

# Set the desired Statistic Output (sst.statOutputConsole is default)
sst.setStatisticOutput("sst.statOutputConsole")
#sst.setStatisticOutput("sst.statOutputTXT", {"filepath" : "./TestOutput.txt"
#                                            })
#sst.setStatisticOutput("sst.statOutputCSV", {"filepath" : "./TestOutput.csv",
#                                                         "separator" : ", "
#                                            })

# Enable Individual Statistics for the Component with output at end of sim
# Statistic defaults to Accumulator
ariel.enableStatistics([
      "cycles",
      "active_cycles",
      "instruction_count",
      "read_requests",
      "write_requests"
])

l1cache.enableStatistics([
      "CacheHits",
      "CacheMisses"
])

