#!/bin/bash

if [ -z "${NETSIM_CMAKE_FLAGS}" ]; then
    NETSIM_CMAKE_FLAGS=
fi

if [ -z "${NETSIM_MAKE_FLAGS}" ]; then
    NETSIM_MAKE_FLAGS=
fi

set -e -u
mkdir -p build

echo "Configuring..."
echo "cmake .. ${NETSIM_CMAKE_FLAGS}"
pushd build >& /dev/null
cmake .. ${NETSIM_CMAKE_FLAGS}
echo "Building..."
echo "make ${NETSIM_MAKE_FLAGS} $@"
make ${NETSIM_MAKE_FLAGS} $@
popd >& /dev/null
