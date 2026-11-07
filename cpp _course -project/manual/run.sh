#!/bin/sh
# Rebuild for this Mac on each launch, then make examples/ relative paths work.
set -eu

script_dir=$(CDPATH= cd -P "$(dirname "$0")" && pwd)
sh "$script_dir/build.sh"
cd "$script_dir"

# The program uses the last --data option, so a caller can override the default.
exec "$script_dir/bin/manual_grade" --data "$script_dir/data" "$@"
