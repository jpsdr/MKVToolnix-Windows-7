#!/usr/bin/ruby -w

# T_599mp4_nclx_colour_type_in_colr_atom
describe "mkvmerge / MP4 files with `nclx` colour type in the `colr` atom"
test_merge "data/mp4/colr_atom_with_nclx_colour_type.mp4"
