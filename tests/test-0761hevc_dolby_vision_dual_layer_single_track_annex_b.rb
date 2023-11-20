#!/usr/bin/ruby -w

# T_761hevc_dolby_vision_dual_layer_single_track_annex_b
describe "mkvmerge / HEVC Annex B type bitstreams with Dolby Vision dual-layer, both layers in same track"

test_merge "data/h265/dolby-vision/test02_dv_fel.hevc"
test_merge "data/h265/dolby-vision/lg_dolby_comparison_4k_demo_short.ts", :args => "-A"
