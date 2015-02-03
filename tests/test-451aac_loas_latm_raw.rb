#!/usr/bin/ruby -w

# T_451aac_loas_latm_raw
describe "mkvmerge / AAC in raw LOAS/LATM multiplex"
test_merge "data/aac/aac_loas_latm.aac", :exit_code => :warning
