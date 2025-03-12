#!/usr/bin/ruby -w

# T_774issue_3288_96000hz_dtshd_hr
describe "mkvmerge / issue 3288: DTS-HD High Resolution with X96 extension, report as 96kHz"

files = [
  [ 0, "data/dts/issue_3288_96000hz_DTSHD-HR.dtshd", ],
  [ 1, "data/dts/issue_3288_96000hz_DTS-HRA.mkv",    ],
]

files.each do |file|
  test_merge(file[1], :args => "-D -S -a #{file[0]}")

  test "identification" do
    fail if identify_json(file[1])["tracks"][file[0]]["properties"]["audio_sampling_frequency"] != 96000

    "OK"
  end
end
