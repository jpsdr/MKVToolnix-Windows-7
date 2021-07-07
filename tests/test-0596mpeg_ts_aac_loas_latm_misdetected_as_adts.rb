#!/usr/bin/ruby -w

# T_596mpeg_ts_aac_loas_latm_misdetected_as_adts
describe "mkvmerge / MPEG TS AAC with LOAS/LATM multiplex detected as ADTS multiplex"
test_merge "data/ts/aac_loas_latm_misdetected_as_adts.ts", :args => "--no-subtitles --no-video"
