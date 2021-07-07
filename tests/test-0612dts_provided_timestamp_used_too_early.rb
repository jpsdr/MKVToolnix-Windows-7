#!/usr/bin/ruby -w

# T_612dts_provided_timestamp_used_too_early
describe "mkvmerge / DTS, provided timestamp is used too early (bug 2071)"
test_merge "data/dts/laced_no_default_duration.mka"
