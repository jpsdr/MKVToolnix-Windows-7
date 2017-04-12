#!/usr/bin/ruby -w

# T_592mpeg_ts_aac_wrong_track_parameters_detected
describe "mkvmerge / MPEG TS, AAC: false positive during AAC header detection leading to wrong track parameters"
test_merge "data/h265/wrong_number_of_parameter_sets_in_hevcc.ts", :args => "--no-video"
