In order to use the Ramulator2 memory backend in SST, the following steps must be taken (until we are able to merge our own external frontend into Ramulator 2):

1. Go to https://github.com/CMU-SAFARI/ramulator2 and clone the repository
2. Take the file `<path-to-sst-elements-src>/src/sst/elements/memHierarchy/membackend/ramulator2/sst_frontend.cpp` and copy it into the following directory: `<path-to-ramulator2>/src/frontend/impl/external_wrapper`
3. Open `<path-to-ramulator2>/src/frontend/CMakeLists.txt` and add `impl/external_wrapper/sst_frontend.cpp` to the `target_sources`
4. Follow the instructions to configure and build Ramulator 2
5. When configuring SST Elements, use the `--with-ramulator2=<path-to-ramulator2>` argument alongside any other configuration options

If you would like an example of configuration files, see `sst-elements/src/sst/elements/memHierarchy/tests/ramulator2-ddr4.cfg`, `sst-elements/src/sst/elements/memHierarchy/tests/ramulator2-test.py`, and `sst-elements/src/sst/elements/memHierarchy/tests/sdl4-2-ramulator2.py`. A Ramulator config file like `ddr4-ramulator2.cfg` must be passed into the backend to use Ramulator 2 with SST.