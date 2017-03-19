#!/usr/bin/ruby -w

# T_586h265_invalid_default_display_window_in_sps_vui
describe "mkvmerge / h.265 with an invalid default display window in the VUI parameters of the sequence parameter sets"
test_merge "data/h265/invalid_default_display_window_in_sps_vui.h265"
