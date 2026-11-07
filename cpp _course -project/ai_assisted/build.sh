#!/bin/sh
# Build the current sources without replacing a working binary on failure.
set -eu

case $0 in
    /*) script_path=$0 ;;
    *) script_path=$(pwd -P)/$0 ;;
esac
script_dir=$(CDPATH= cd -P "$(dirname "$script_path")" && pwd -P)

if [ "$(uname -s)" = Darwin ]; then
    if ! command -v xcode-select >/dev/null 2>&1 ||
       ! xcode-select -p >/dev/null 2>&1 ||
       ! command -v xcrun >/dev/null 2>&1 ||
       ! xcrun --find clang++ >/dev/null 2>&1; then
        printf '%s\n' '未找到可用的 Apple Command Line Tools。' >&2
        printf '%s\n' '请在终端手动运行 xcode-select --install，完成安装后重试。' >&2
        printf '%s\n' '本脚本不会自动安装工具，也不会替换已有可执行文件。' >&2
        exit 1
    fi
fi

compiler=${CXX:-clang++}
if ! command -v "$compiler" >/dev/null 2>&1; then
    printf '%s\n' "找不到 C++ 编译器：$compiler" >&2
    printf '%s\n' 'macOS 请先安装 Command Line Tools：xcode-select --install' >&2
    printf '%s\n' '也可用 CXX 指定一个编译器可执行文件的名称或完整路径。' >&2
    exit 1
fi

mkdir -p "$script_dir/bin"
build_tmp=$(mktemp "$script_dir/bin/.ai_grade.XXXXXXXX")
trap 'rm -f "$build_tmp"' 0
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

printf '%s\n' "正在使用 $compiler 编译 C++17 程序……" >&2
if ! "$compiler" -std=c++17 -O2 -Wall -Wextra -Wpedantic \
    "$script_dir/src/main.cpp" -o "$build_tmp"; then
    printf '%s\n' '编译失败，已有 bin/ai_grade 保持不变。请检查上方诊断。' >&2
    exit 1
fi
chmod +x "$build_tmp"
mv -f "$build_tmp" "$script_dir/bin/ai_grade"
printf '%s\n' "编译完成：$script_dir/bin/ai_grade" >&2
