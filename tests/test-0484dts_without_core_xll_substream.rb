#!/usr/bin/ruby -w

# T_484dts_without_core_xll_substream
describe "mkvmerge / DTS file without cores, only XLL extension substreams"
test_merge "data/dts/xll-no-core.dts"
