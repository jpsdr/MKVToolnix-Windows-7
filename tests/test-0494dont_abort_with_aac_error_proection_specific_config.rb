#!/usr/bin/ruby -w

# T_494dont_abort_with_aac_error_proection_specific_config
describe "mkvmerge / don't abort file identification with aac_error_proection_specific_config"
test_identify "data/h264/error-during-aac-detection.h264"
