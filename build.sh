#!/bin/bash

if [ -z "${MODEMSIM_CMAKE_FLAGS}" ]; then
    MODEMSIM_CMAKE_FLAGS=
fi

if [ -z "${MODEMSIM_MAKE_FLAGS}" ]; then
    MODEMSIM_MAKE_FLAGS=
fi

set -e -u
mkdir -p build

echo "Configuring..."
echo "cmake .. ${MODEMSIM_CMAKE_FLAGS}"
pushd build >& /dev/null
cmake .. ${MODEMSIM_CMAKE_FLAGS}
echo "Building..."
echo "make ${MODEMSIM_MAKE_FLAGS} $@"
make ${MODEMSIM_MAKE_FLAGS} $@
popd >& /dev/null
