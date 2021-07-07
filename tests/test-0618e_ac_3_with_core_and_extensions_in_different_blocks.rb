#!/usr/bin/ruby -w

# T_618e_ac_3_with_core_and_extensions_in_different_blocks
describe "mkvmerge / E-AC-3 in Matroska with core & extension in different blocks"
test_merge "data/ac3/e-ac-3_with_core_and_extensions_in_different_blocks.mkv", :args => "--no-video --no-subtitles --disable-lacing"
