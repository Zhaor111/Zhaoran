#!/bin/sh
# Build on every launch; resolve relative imports and --data paths from this project.
set -eu

case $0 in
    /*) script_path=$0 ;;
    *) script_path=$(pwd -P)/$0 ;;
esac
script_dir=$(CDPATH= cd -P "$(dirname "$script_path")" && pwd -P)

sh "$script_dir/build.sh"
cd "$script_dir"
exec "$script_dir/bin/ai_grade" --data "$script_dir/data" "$@"
