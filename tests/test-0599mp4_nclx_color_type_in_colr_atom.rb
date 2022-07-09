#!/usr/bin/ruby -w

# T_599mp4_nclx_color_type_in_colr_atom
describe "mkvmerge / MP4 files with `nclx` color type in the `colr` atom"
test_merge "data/mp4/colr_atom_with_nclx_color_type.mp4"
