#!/bin/sh
set -eu

root=$1
project=$2
source=$root/src/meson.build

overrides=$(grep -Fc 'meson.override_dependency(' "$source")
if [ "$overrides" -ne 1 ]; then
    echo "subproject-contract: dependency override cardinality is $overrides, expected 1" >&2
    exit 1
fi

if ! grep -Fq "'$project'" "$source"; then
    echo "subproject-contract: dependency override does not name $project" >&2
    exit 1
fi
