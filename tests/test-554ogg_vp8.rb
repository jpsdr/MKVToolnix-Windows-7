#!/usr/bin/ruby -w

# T_554ogg_vp8
describe "mkvmerge / VP8 in Ogg, wrong timestamp calculation and absent Vorbis comment header"
test_merge "data/vp8/a_V.ogv"
test_merge "data/vp8/ogg-vp8-without-tags.ogv"
