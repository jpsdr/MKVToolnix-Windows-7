#!/usr/bin/ruby -w

# T_783aac_loas_latm_not_starting_on_a_frame_boundary
describe "mkvmerge / AAC LOAS/LATM stream not starting on a frame boundary"
test_merge "data/aac/loas_latm_leading_garbage.aac"
