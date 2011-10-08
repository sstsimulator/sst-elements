#!/bin/bash
if [[ $1 == "clean" ]] ; then
    rm ghost
    exit
fi

mpicc -Wall ghost.c  memory.c  neighbors.c  ranks.c  work.c -o ghost -lm
