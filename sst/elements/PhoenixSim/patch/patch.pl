#!/bin/bash
echo "BALf"
find $1 -name "csimulation.h" -exec sed -i 's/void insertMsg/virtual void insertMsg/g' {} \;


