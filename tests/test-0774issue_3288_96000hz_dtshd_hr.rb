#!/usr/bin/ruby -w

# T_774issue_3288_96000hz_dtshd_hr
describe "mkvmerge / issue 3288: DTS-HD High Resolution with X96 extension, report as 96kHz"

file_name = "data/dts/issue_3288_96000hz_DTSHD-HR.dtshd"

test_merge(file_name)

test "identification" do
  fail if identify_json(file_name)["tracks"][0]["properties"]["audio_sampling_frequency"] != 96000

  "OK"
end
