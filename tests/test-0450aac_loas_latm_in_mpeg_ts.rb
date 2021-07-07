#!/usr/bin/ruby -w

# T_450aac_loas_latm_in_mpeg_ts
describe "mkvmerge / AAC in LOAS/LATM multiplex inside MPEG transport streams"

test_merge "data/ts/avc_aac_loas_latm.ts", :args => "--no-video"
