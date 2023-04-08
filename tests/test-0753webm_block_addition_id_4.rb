#!/usr/bin/ruby -w

# T_753webm_block_addition_id_4
describe "mkvmerge / WebM with block addition ID 4, auto-creating mappings for it"

test_merge "data/mkv/hdr10_plus_vp9_sample_ffmpeg_output.mkv"
test_merge "data/webm/hdr10_plus_vp9_sample.webm"
test_merge "data/mkv/hdr10_plus_vp9_sample_ffmpeg_output.mkv", :args => "--webm"
test_merge "data/webm/hdr10_plus_vp9_sample.webm",             :args => "--webm"
