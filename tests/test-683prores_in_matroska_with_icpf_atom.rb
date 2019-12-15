#!/usr/bin/ruby -w

# T_683prores_in_matroska_with_icpf_atom
describe "mkvmerge / ProRes in Matroska with 'icpf' atom header in frames"
test_merge "data/mkv/prores_with_icpf_atom.mkv"
