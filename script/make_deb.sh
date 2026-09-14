#! /bin/bash
# 获取脚本所在目录，然后切换到项目根目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR/.."
PROJECT_ROOT="$(pwd)"
build_path="$PROJECT_ROOT"
project_list=("rw101" "rw350r" "rw135")
oem_list=("dell" "hp" "lenovo" "generic")

# 1表示使用库编译，0表示正常全源码编译（opensrc：deb/rpm 仅支持库编译）
build_by_lib=0

parse_ini() {
    local inifile=$1 section=$2 item=$3
    grep -E "^\[$section\]" -A1000 "$inifile" | grep -E "^\[$section\]|\b$item\b" \
    | grep -vE "^\[$section\]|^\s*$" | awk -F'=' 'NR==1{print $2}' | tr -d ' \n'
}

function run_build_lib_targets()
{
    local project=$1 oem=$2
    local build_ma build_ma_lib
    build_ma=$(parse_ini "${PROJECT_ROOT}/${project}_config" "$oem" BUILD_MA)
    build_ma_lib=$(parse_ini "${PROJECT_ROOT}/${project}_config" "$oem" BUILD_MA_LIB)
    make helper_build_lib
    make flash_build_lib
    make config_build_lib
    if [ "$build_ma_lib" = "yes" ] || [ "$build_ma" = "yes" ]; then
        make ma_build_lib
    fi
}

function ensure_ma_artifact()
{
    local project=$1 oem=$2
    local build_ma build_ma_lib
    build_ma=$(parse_ini "${PROJECT_ROOT}/${project}_config" "$oem" BUILD_MA)
    build_ma_lib=$(parse_ini "${PROJECT_ROOT}/${project}_config" "$oem" BUILD_MA_LIB)
    if [ "$build_ma_lib" != "yes" ] && [ "$build_ma" != "yes" ]; then
        return 0
    fi
    if [ ! -d "build" ]; then
        return 0
    fi
    cmake --build build --target ma_build_lib || cmake --build build --target rolling_ma
}

function sync_versioned_rolling_ma()
{
    local project=$1 oem=$2
    local ma_bin="build/application/rolling_ma_service/rolling_ma"
    if [ -z "${DISTRO_TAG:-}" ] || [ ! -f "${ma_bin}" ]; then
        return 0
    fi
    mkdir -p "binary_deb/common_lib-${DISTRO_TAG}"
    cp -f "${ma_bin}" "binary_deb/common_lib-${DISTRO_TAG}/rolling_ma"
    if [ "${build_by_lib}" -eq 1 ]; then
        mkdir -p binary_deb/common_lib
        cp -f "${ma_bin}" binary_deb/common_lib/rolling_ma
    fi
}

function cmake_distro_tag_args()
{
    if [ -n "${DISTRO_TAG:-}" ]; then
        echo "-DDISTRO_TAG=${DISTRO_TAG}"
    fi
}

function parse_by_lib_flag()
{
    for arg in "$@"; do
        if [ "$arg" = "--by-lib" ]; then
            build_by_lib=1
        fi
    done
}

function require_prebuilt_common_lib()
{
    if [ ! -d "binary_deb/common_lib" ] || [ -z "$(ls -A binary_deb/common_lib 2>/dev/null)" ]; then
        echo "ERROR: binary_deb/common_lib is missing or empty." >&2
        echo "This opensrc tree only supports packaging deb/rpm from prebuilt static libraries." >&2
        echo "Copy libs from the full dev tree, or run: ./script/build_multi_distro.sh lib <project> <oem> <distro>" >&2
        exit 1
    fi
}

function make_project()
{
    local pkg_type=$1 project=$2 oem=$3
    current_path="$PROJECT_ROOT"
    # 保留 binary_deb/common_lib 与 common_lib-*（多发行版 Docker 产物），清掉其它内容
    if [ -d "binary_deb" ]; then
        if [ "$build_by_lib" -eq 1 ] && [ -d "binary_deb/common_lib" ]; then
            echo "build_by_lib=1: keep common_lib and common_lib-*"
            find binary_deb -mindepth 1 -maxdepth 1 \
                -not -name "common_lib" \
                -not -name "common_lib-*" \
                -exec rm -rf {} \;
        else
            echo "keep versioned common_lib-* under binary_deb"
            find binary_deb -mindepth 1 -maxdepth 1 \
                -not -name "common_lib-*" \
                -exec rm -rf {} \;
            mkdir -p ${current_path}/binary_deb
        fi
    else
        mkdir ${current_path}/binary_deb
    fi
    if [ -d "build" ]; then
        rm -rf build
    fi
    if [ -n "$pkg_type" ] && [ -n "$project" ] && [ -n "$oem" ]; then
        local distro_tag_args
        distro_tag_args=$(cmake_distro_tag_args)
        if [ "deb" == "$pkg_type" ]; then
            cmake -S . -B build -DBUILD_DEB=yes -DPROJECT_BUILD=$project -DOEM_BUILD=$oem -DBUILD_BY_LIB=$build_by_lib ${distro_tag_args}
        elif [ "rpm" == "$pkg_type" ]; then
            cmake -S . -B build -DBUILD_RPM=yes -DPROJECT_BUILD=$project -DOEM_BUILD=$oem -DBUILD_BY_LIB=$build_by_lib ${distro_tag_args}
        else
            cmake -S . -B build -DPROJECT_BUILD=$project -DOEM_BUILD=$oem -DBUILD_BY_LIB=$build_by_lib ${distro_tag_args}
        fi
        cmake --build build
        if [ "$build_by_lib" -ne 1 ]; then
            ensure_ma_artifact "$project" "$oem"
            sync_versioned_rolling_ma "$project" "$oem"
        fi
        cd build
        cpack
        if [ "deb" == "$pkg_type" ]; then
            mv *.deb ${current_path}/binary_deb/ 2>/dev/null || true
        elif [ "rpm" == "$pkg_type" ]; then
            mv *.rpm ${current_path}/binary_deb/ 2>/dev/null || true
        fi
        cd ../
    else
        for project in "${project_list[@]}"; do
            for oem in "${oem_list[@]}"; do
                if [ -d "build" ]; then
                    rm -rf build
                fi
                cmake -S . -B build -DPROJECT_BUILD=${project} -DOEM_BUILD=${oem} -DBUILD_BY_LIB=$build_by_lib
                cmake --build build
                cd build
                cpack
                mv *.deb ${current_path}/binary_deb/
                cd ../

                cmake -S . -B build -DBUILD_PACKAGE=rpm -DPROJECT_BUILD=${project} -DOEM_BUILD=${oem} -DBUILD_BY_LIB=$build_by_lib
                cmake --build build
                cd build
                cpack
                mv *.rpm ${current_path}/binary_deb/
                cd ../
            done
        done
    fi
}

function make_biny_lib()
{
    current_path="$PROJECT_ROOT"
    if [ -d "build" ]; then
        rm -rf build
    fi
    local preserve_common_lib=0
    local common_lib_backup=""
    if [ $# -eq 2 ] && [ -d "binary_deb/common_lib" ]; then
        preserve_common_lib=1
        common_lib_backup=$(mktemp -d)
        cp -a binary_deb/common_lib/. "${common_lib_backup}/"
    fi
    if [ -d "binary_deb" ]; then
        rm -rf binary_deb 2>/dev/null || sudo rm -rf binary_deb
    fi
    mkdir -p ${current_path}/binary_deb

    if [ $# -eq 2 ]; then
        local distro_tag_args
        distro_tag_args=$(cmake_distro_tag_args)
        cmake -S . -B build -DBUILD_LIB=yes -DPROJECT_BUILD=$1 -DOEM_BUILD=$2 ${distro_tag_args}
        cmake --build build
        cd build
        run_build_lib_targets $1 $2
        cp -raf common_lib ${current_path}/binary_deb/
        if [ "$preserve_common_lib" -eq 1 ]; then
            cp -an "${common_lib_backup}/." ${current_path}/binary_deb/common_lib/
            rm -rf "${common_lib_backup}"
        fi
        cd ../
        sync_versioned_rolling_ma "$1" "$2"
    else
        for project in "${project_list[@]}"; do
            for oem in "${oem_list[@]}"; do
                if [ -d "build" ]; then
                    rm -rf build
                fi
                cmake -S . -B build -DBUILD_LIB=yes -DPROJECT_BUILD=${project} -DOEM_BUILD=${oem}
                cmake --build build
                cd build
                run_build_lib_targets ${project} ${oem}
                cp -raf common_lib ${current_path}/binary_deb/
                cd ../
            done
        done
    fi
}

function modify_apps_version()
{
    version="$1"
    awk -v version="$version" '
    {
        if ($0 ~ /project\(rolling_linux VERSION .*\)/) {
            gsub(/project\(rolling_linux VERSION .*\)/, "project(rolling_linux VERSION " version ")")
        }
        print
    }
    ' ${build_path}/CMakeLists.txt > ${build_path}/tmp
    mv ${build_path}/tmp ${build_path}/CMakeLists.txt
}

# 解析 --by-lib 标志
parse_by_lib_flag "$@"

if [[ "deb" == "$1" ]] || [[ "rpm" == "$1" ]]; then
    build_by_lib=1
    require_prebuilt_common_lib
    filtered_args=()
    for arg in "$@"; do
        if [ "$arg" != "--by-lib" ]; then
            filtered_args+=("$arg")
        fi
    done
    set -- "${filtered_args[@]}"
    if [ $# -eq 3 ]; then
        make_project $1 $2 $3
    elif [ $# -eq 4 ]; then
        version_regex='^[0-9]+\.[0-9]+'
        if [[ $4 =~ $version_regex ]]; then
            modify_apps_version $4
        fi
        make_project $1 $2 $3
    else
        make_project
    fi
elif [ "lib" == "$1" ]; then
    if [ $# -eq 3 ]; then
         make_biny_lib $2 $3
    else
        make_biny_lib
    fi
else
    echo  -e "para error,for example, you can send cmd to build deb (requires binary_deb/common_lib)
     \033[1;31m ./make_deb.sh deb rw350r lenovo
     \033[1;31m ./make_deb.sh deb rw350r dell
     \033[1;31m ./make_deb.sh rpm rw350r lenovo
     \033[0;30m or you can send cmd build helper lib
     \033[1;31m ./make_deb.sh lib rw350r lenovo"
fi
