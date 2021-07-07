#!/usr/bin/ruby -w

# T_615opus_discard_padding_in_the_middle
describe "mkvmerge / Opus with frames with discard padding in the middle"
test_merge "data/opus/discard-padding-in-the-middle.opus"
