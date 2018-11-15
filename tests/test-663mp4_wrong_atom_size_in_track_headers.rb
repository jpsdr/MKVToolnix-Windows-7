#!/usr/bin/ruby -w

# T_663mp4_wrong_atom_size_in_track_headers
describe "mkvmerge / MP4: wrong atom size (exceeding parent atom's size) in track headers"

test_merge "data/mp4/wrong_atom_size_in_track_headers.mp4"
