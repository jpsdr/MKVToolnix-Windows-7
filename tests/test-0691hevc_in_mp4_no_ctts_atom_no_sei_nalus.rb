#!/usr/bin/ruby -w

# T_691hevc_in_mp4_no_ctts_atom_no_sei_nalus
describe "mkvmerge / HEVC in MP4 without ctts atom but no B frames either, no SEI NALUs in hevcC"

test_merge "data/h265/no_ctts_atom_no_b_frames.mp4"
