#!/usr/bin/ruby -w

# T_568dts_wav_14_in_16_detection_failures
describe "mkvmerge / DTS in WAV, failure in proper detection of byte swapping/14-in-16 packing"

test_merge "data/wav/05-labyrinth-path-13.wav"
test_merge "data/wav/06-labyrinth-path-14.wav"
