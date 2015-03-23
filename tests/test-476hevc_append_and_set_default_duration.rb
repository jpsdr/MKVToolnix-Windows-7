#!/usr/bin/ruby -w

# T_476hevc_append_and_set_default_duration
describe "mkvmerge / append HEVC tracks and setting the default duration"
test_merge "--default-duration 0:100fps data/h265/user_data.hevc + data/h265/user_data.hevc"
