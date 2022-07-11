#/bin/sh
set -eux

LIB_DIR=springql_client-x86_64-unknown-linux-gnu-debug

gcc main.c -g -O3 -lspringql_client -L${LIB_DIR}

# LD_LIBRARY_PATH=${LIB_DIR} valgrind --leak-check=full --leak-resolution=high --show-possibly-lost=no ./a.out
LD_LIBRARY_PATH=${LIB_DIR} ./a.out
