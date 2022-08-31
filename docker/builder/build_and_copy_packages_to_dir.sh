#! /bin/bash -e

usage() {
cat <<_EOF_
Builds C++ REST Client for iRODS.

Available options:

    -d, --debug             Build with symbols for debugging
    -j, --jobs              Number of jobs for make tool
    -N, --ninja             Use ninja builder as the make tool
    -h, --help              This message
_EOF_
    exit
}

if [[ -z ${file_extension} ]] ; then
    echo "\$file_extension not defined"
    exit 1
fi

make_program="make"
make_program_config=""
build_jobs=0
debug_config="-DCMAKE_BUILD_TYPE=Release"

while [ -n "$1" ] ; do
    case "$1" in
        -N|--ninja)              make_program_config="-GNinja";
                                 make_program="ninja";;
        -j|--jobs)               shift; build_jobs=$(($1 + 0));;
        -d|--debug)              debug_config="-DCMAKE_BUILD_TYPE=Debug";;
        -h|--help)               usage;;
    esac
    shift
done

common_cmake_args=(
    -DCMAKE_COLOR_MAKEFILE=ON
    -DCMAKE_VERBOSE_MAKEFILE=ON
    -DIRODS_BUILD_WITH_WERROR=False
)

build_jobs=$(( !build_jobs ? $(nproc) - 1 : build_jobs )) #prevent maxing out CPUs

echo "========================================="
echo "beginning build"
echo "========================================="

# Build iRODS
mkdir -p /build && cd /build
cmake ${make_program_config} ${debug_config} "${common_cmake_args[@]}" /src
if [[ -z ${build_jobs} ]] ; then
    ${make_program} package
else
    echo "using [${build_jobs}] threads"
    ${make_program} -j ${build_jobs} package
fi

# Copy packages to mounts
cp -r /build/*."${file_extension}" /packages/
