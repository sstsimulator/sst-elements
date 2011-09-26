#!/bin/bash
mpicc -Wall ghost.c  memory.c  neighbors.c  ranks.c  work.c -o ghost
