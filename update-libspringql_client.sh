#!/bin/sh
set -eux

TOP_DIR=$(cd $(dirname $0); pwd)

SPRINGQL_VERSION=$1

for target_triple in x86_64-unknown-linux-gnu aarch64-unknown-linux-gnu ; do
    for build_type in debug release ; do
        SPRINGQL_URL="https://github.com/SpringQL/SpringQL-client-c/releases/download/${SPRINGQL_VERSION}/springql_client-${target_triple}-${build_type}.zip"
        SPRINGQL_ZIP=$(basename $SPRINGQL_URL)
        SPRINGQL_DIR=$(basename $SPRINGQL_URL .zip)

        (
            cd $TOP_DIR

            wget $SPRINGQL_URL
            unzip -o $SPRINGQL_ZIP
            mv -f $SPRINGQL_DIR/springql.h .

            rm -f $SPRINGQL_ZIP*
        )
    done
done
