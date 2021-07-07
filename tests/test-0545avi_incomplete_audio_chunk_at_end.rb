#!/usr/bin/ruby -w

# T_545avi_incomplete_audio_chunk_at_end
describe "mkvmerge / reading an AVI that ends in the middle of an audio chunk"
test_merge "data/avi/incomplete-audio-chunk-at-end.avi"
