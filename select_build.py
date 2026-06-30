#!/usr/bin/env python3

import sys
import os
import subprocess
from pathlib import Path

# This script modifies the sst-elements build system using .ignore files
# and reruns autogen.sh. Run with no arguments or 'help' to see usage.


# Map dependencies between libraries where:
#   Some of these are 'required' in the sense that the library is not useful without the others
#   Some of these are just almost always used together but not strictly required
#
# The dependency sets are fully expanded so we don't have to recursively look up deps of deps
deps = {
    "ariel"         : {"ariel", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "balar"         : {"balar", "cassini", "kingsley", "memhierarchy", "merlin", "mmu", "shogun", "vanadis"},
    "cachetracer"   : {"cachetracer", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "carcosa"       : {"carcosa", "cassini", "kingsley", "memhierarchy", "merlin", "mmu", "shogun", "vanadis"},
    "cramsim"       : {"cramsim", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"}, # Will also need CPU
    "cassini"       : {"cassini", "kingsley", "memhierarchy", "merlin", "shogun"}, # Will also need CPU
    "ember"         : {"ember", "firefly", "hermes", "merlin"},
    "firefly"       : {"firefly", "ember", "hermes", "merlin"},
    "gensa"         : {"gensa", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "golem"         : {"golem", "cassini", "kingsley", "memhierarchy", "merlin", "mmu", "shogun", "vanadis"},
    "hermes"        : {"ember", "hermes", "firefly", "merlin"},
    "iris"          : {"iris", "mask-mpi", "mercury", "merlin"},
    "kingsley"      : {"kingsley"},
    "llyr"          : {"cassini", "kingsley", "llyr", "memhierarchy", "merlin", "shogun"},
    "mask-mpi"      : {"iris", "mask-mpi", "mercury"},
    "memhierarchy"  : {"memhierarchy", "cassini", "kingsley", "merlin", "shogun"}, # Will also need CPU
    "mercury"       : {"mercury", "iris", "mask-mpi"},
    "merlin"        : {"merlin"},
    "messier"       : {"cassini", "kingsley", "memhierarchy", "merlin", "messier", "shogun"},
    "miranda"       : {"cassini", "kingsley", "memhierarchy", "merlin", "miranda", "shogun"},
    "mmu"           : {"cassini", "kingsley", "memhierarchy", "merlin", "mmu", "shogun", "vanadis"},
    "opal"          : {"opal", "ariel", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "osseous"       : {"osseous", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "prospero"      : {"prospero", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "rdmanic"       : {"rdmanic", "vanadis", "mmu", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "samba"         : {"samba", "ariel", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "shogun"        : {"shogun", "cassini", "memhierarchy"}, # Will also need CPU
    "simpleelementexample" : {"simpleelementexample"},
    "simplesimulation" : {"simplesimulation"},
    "thornhill"     : {"thornhill", "miranda", "cassini", "kingsley", "memhierarchy", "merlin", "shogun", "ember"},
    "vaultsim"      : {"vaultsim", "cassini", "kingsley", "memhierarchy", "merlin", "shogun"},
    "serrano"       : {"serrano"},
    "vanadis"       : {"cassini", "kingsley", "memhierarchy", "merlin", "mmu", "rdmanic", "shogun", "vanadis"},
    "zodiac"        : {"ember", "firefly", "hermes", "merlin", "zodiac"}
}

# Add some more special-case options
deps["network"] = deps["ember"]
deps["network-ext"] = deps["network"] | deps["mercury"]
deps["network-all"] = deps["network-ext"] | deps["zodiac"]
deps["accel"] = deps["balar"] | deps["llyr"] | deps["golem"] | deps["osseous"] | deps["gensa"]
deps["cpu"] = deps["ariel"] | deps["miranda"] | deps["prospero"] | deps["vanadis"]
deps["mem"] = deps["memhierarchy"] | deps["vaultsim"] | deps["cramsim"] | deps["messier"]
deps["node"] = deps["cpu"] | deps["mem"]
deps["example"] = deps["simpleelementexample"] | deps["simplesimulation"]
# All libraries that can be used for node-level simualtion
deps["node-all"] = deps["cpu"] | deps["accel"] | deps["mem"] | deps["cachetracer"] | deps["carcosa"] | deps["opal"] | deps["samba"]

###### Main

if len(sys.argv) == 1 or sys.argv[1] == "--help" or sys.argv[1] == "help" or sys.argv[1] == "-h":
    print("This script will modify the sst-elements build so that only selected libraries are built.\n"
          "Pass the list of desired libraries to this script as space-separated names. Names are *not* case-sensitive.\n"
          "Dependencies or libraries commonly needed for the requested libraries will also be built. All others will be disabled.\n"
          "This script will rerun autogen.sh once the build is modified and therefore requires autotools to be installed.\n"
          "In addition to library names, the following categories can be requested:\n"
          "  example     - all example libraries\n"
          "  network     - basic libraries needed for an ember-based system network simulation\n"
          "  network-ext - libraries needed for ember- and mercury-based system simulations\n"
          "  network-all - all libraries that can be used for system network simulations\n"
          "  node        - CPU, NoC, and basic memory models\n"
          "  accel       - accelerator models, memory system, NoC, and Vanadis CPU model\n"
          "  mem         - various main memory models\n"
          "  node-all    - all libraries that can be used for node-level simulation\n"
          "  all         - enable all available libraries"
    )
    sys.exit()

# Parse arguments and build list of requested libraries

args = sys.argv[1:]
libs = set()
for arg in args:
    arglow = arg.lower()
    if arglow == "all":
        libs = set(list(deps))
    else:
        new_deps = deps[arglow]
        if new_deps == None:
            print("Error: requested library '{}' is not found in the dependencies list. Build will not be modified.".format(arg))
            sys.exit()
        libs.update(new_deps)

print("Script will enable the following libraries: ")
print(*libs, sep=", ")

# Warnings about possibly missing libraries
# Maybe these are intentional but maybe not
## Check: Detect presence of memHierarchy without a CPU model
if "memhierarchy" in libs:
    cpus = libs & { "ariel", "miranda", "prospero", "vanadis" }
    if not len(cpus):
        print("\nDetected memHierarchy but no CPU models in the enable list. "
              "If a CPU model is desired, add one of [ariel, miranda, prospero, vanadis] or 'cpu' for all.")



# Make sure user is happy with the proposed build before actually modifying it
choice = input("Press ENTER to continue or type 'q' and press ENTER to quit without modifying the build: ").strip().lower()

if choice == 'q':
    sys.exit()

# Iterate through elements, add .ignore if not in libs, remove .ignore if in libs
dir_path = Path("src/sst/elements")

avail_elements = [x.name for x in dir_path.iterdir() if x.is_dir()]

for elem in avail_elements:
    file_path = Path("src/sst/elements/" + elem + "/.ignore")
    if elem.lower() in libs:
        # Enable (remove .ignore if it exists)
        file_path.unlink(missing_ok=True)
    else:
        # Disable (add .ignore if it doesn't already exist)
        file_path.touch()

# Run autogen.sh to generate the build
subprocess.run(["sh", "autogen.sh"])