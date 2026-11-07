#!/bin/sh
set -eu
PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
sh "$PROJECT_DIR/manual/build.sh"
sh "$PROJECT_DIR/ai_assisted/build.sh"
printf '\n两套系统均已编译完成。\n'
