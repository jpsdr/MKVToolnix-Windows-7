#!/usr/bin/ruby -w

# T_767aac_latm_audio_specific_config_late
describe "mkvmerge / AAC LOAS/LATM stream, AudioSpecificConfig comes in later frame"
test_merge "data/aac/aac_lc_latm_problem.aac"
