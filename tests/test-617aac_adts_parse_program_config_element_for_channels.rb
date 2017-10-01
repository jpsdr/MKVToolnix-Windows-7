#!/usr/bin/ruby -w

# T_617aac_adts_parse_program_config_element_for_channels
describe "mkvmerge / AAC in ADTS with channel_config == 0, but with program_config_element at start signalling 7.1 channels"
test_merge "data/aac/adts_0_channels_but_program_config_element_at_start.aac"
