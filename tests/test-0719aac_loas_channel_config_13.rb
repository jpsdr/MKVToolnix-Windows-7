#!/usr/bin/ruby -w

# T_719aac_loas_channel_config_13
describe "mkvmerge / LOAS/LATM AAC with channel config ≥ 8"

[ [ "13-nishino", 24, 48000 ], [ "13-sun", 24, 48000 ] ].each do |spec|
  file_name = "data/aac/channel-config-#{spec[0]}.aac"

  test_merge file_name

  test "identify #{file_name}" do
    props = identify_json(file_name)["tracks"][0]["properties"]
    ok    = props["audio_channels"] == spec[1] and props["audio_sampling_frequency"] == spec[2]

    fail "audio specs don't match #{spec[1]} @ #{spec[2]}" if not ok
    true
  end
end
