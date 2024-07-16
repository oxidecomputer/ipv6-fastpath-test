#!/bin/bash

set -e
set -x

clang -std=c99 -Wall rx.c -o rx -l socket
clang -std=c99 -Wall tx.c -o tx -l dlpi
