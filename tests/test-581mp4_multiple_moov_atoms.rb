#!/usr/bin/ruby -w

# T_581mp4_multiple_moov_atoms
describe "mkvmerge / MP4 DASH with multiple 'moov' atoms"
test_merge "data/mp4/multiple_moov_atoms.mp4"
