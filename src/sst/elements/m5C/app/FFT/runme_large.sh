#!/bin/sh
fft 8 32768 > output_large.txt
fft 8 32768 -i > output_large.inv.txt
