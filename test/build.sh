#!/bin/bash

set -e
set -x

clang -Wall rx.c -o rx -l socket
clang -Wall tx.c -o tx -l dlpi
