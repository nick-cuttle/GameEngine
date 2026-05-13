#!/usr/bin/env sh

run_eztest() {
    test_build_dir=$1
    test_pattern=$2
    test_label=$3
    helper_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

    # call and use eztest.sh if available
    if command -v eztest.sh >/dev/null 2>&1; then
        if [ -n "$test_pattern" ] && [ -n "$test_label" ]; then
            eztest.sh "$test_build_dir" --test "$test_pattern" --label "$test_label"
        elif [ -n "$test_pattern" ]; then
            eztest.sh "$test_build_dir" --test "$test_pattern"
        elif [ -n "$test_label" ]; then
            eztest.sh "$test_build_dir" --label "$test_label"
        else
            eztest.sh "$test_build_dir"
        fi
    else
        # go directly to eztest.sh
        if [ -f "$helper_dir/eztest.sh" ]; then
            eztest_script="$helper_dir/eztest.sh"
        else
            eztest_script="$helper_dir/../test/eztest.sh"
        fi

        # select correct call based on options
        if [ -n "$test_pattern" ] && [ -n "$test_label" ]; then
            sh "$eztest_script" "$test_build_dir" --test "$test_pattern" --label "$test_label"
        elif [ -n "$test_pattern" ]; then
            sh "$eztest_script" "$test_build_dir" --test "$test_pattern"
        elif [ -n "$test_label" ]; then
            sh "$eztest_script" "$test_build_dir" --label "$test_label"
        else
            sh "$eztest_script" "$test_build_dir"
        fi
    fi
}
