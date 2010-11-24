#!/bin/sh
LD_PRELOAD=/usr/local/lib/libdramsim.so LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib ../../core/sst.x --sdl-file mesmthi.xml --lib-path ../ > OUT."$1"
