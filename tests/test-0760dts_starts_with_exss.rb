#!/usr/bin/ruby -w

# T_760dts_starts_with_exss
describe "mkvmerge / DTS starting with EXSS instead of a core / issue 3602"

test_merge "data/dts/starts-with-exss.dts"
