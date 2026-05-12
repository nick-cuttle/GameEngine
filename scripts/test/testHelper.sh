#!/usr/bin/env sh

run_eztest() {
    test_build_dir=$1
    test_pattern=$2
    test_label=$3
    helper_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

    # Use the prepared PATH command when available; otherwise fall back to the repo-local script so
    # callers can run directly from source checkouts.
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
        if [ -f "$helper_dir/eztest.sh" ]; then
            eztest_script="$helper_dir/eztest.sh"
        else
            eztest_script="$helper_dir/../test/eztest.sh"
        fi

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
