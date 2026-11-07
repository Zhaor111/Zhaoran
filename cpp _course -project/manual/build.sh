#!/bin/sh
# Build beside this script, independent of the caller's current directory.
set -eu

script_dir=$(CDPATH= cd -P "$(dirname "$0")" && pwd)
compiler=${CXX:-clang++}

if [ "$(uname -s)" = "Darwin" ]; then
    if ! /usr/bin/xcode-select -p >/dev/null 2>&1 ||
       ! /usr/bin/xcrun --find clang++ >/dev/null 2>&1; then
        printf '%s\n' \
            '未找到可用的 Apple Command Line Tools。' \
            '请在终端手动运行 xcode-select --install，安装完成后再次运行本脚本。' \
            '本脚本不会自动安装或更改开发工具配置。' >&2
        exit 1
    fi
fi

if ! command -v "$compiler" >/dev/null 2>&1; then
    printf '找不到 C++ 编译器：%s\n' "$compiler" >&2
    printf '%s\n' \
        'macOS 请先运行 xcode-select --install。' \
        '也可设置 CXX 为已安装编译器的完整路径；CXX 只填写一个可执行文件。' >&2
    exit 1
fi

mkdir -p "$script_dir/bin"
temporary=$(mktemp "$script_dir/bin/.manual_grade.build.XXXXXX")
trap 'rm -f "$temporary"' 0
trap 'exit 1' 1 2 3 15

printf '正在使用 %s 编译方案 A……\n' "$compiler"
if "$compiler" -std=c++17 -O2 -Wall -Wextra -Wpedantic \
    "$script_dir/main.cpp" -o "$temporary"; then
    chmod 755 "$temporary"
    mv -f "$temporary" "$script_dir/bin/manual_grade"
    printf '编译成功：%s\n' "$script_dir/bin/manual_grade"
else
    printf '%s\n' \
        '编译失败；原有 bin/manual_grade（若存在）保持不变。' \
        '请检查上方编译器输出。macOS 需要支持 C++17 的 Apple Command Line Tools。' >&2
    exit 1
fi
