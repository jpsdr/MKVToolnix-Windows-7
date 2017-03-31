#!/usr/bin/ruby -w

# T_589h264_forcing_default_duration_in_fields_with_source_matroska
describe "mkvmerge / AVC/h.264 forcing the default duration to a number for fields when the source is in Matroska"

def test_this default_duration_arg, expected_default_duration
  file = "data/mkv/issue1916.mkv"
  args = default_duration_arg.nil? ? "" : "--default-duration 0:#{default_duration_arg}"

  test_merge file, :args => args, :keep_tmp => true

  test "default_duration_arg #{default_duration_arg} expected_default_duration #{expected_default_duration}" do
    actual_default_duration = identify_json(tmp)["tracks"][0]["properties"]["default_duration"]
    "#{expected_default_duration}+#{actual_default_duration}+#{expected_default_duration == actual_default_duration}"
  end
end

test_this "25p",  40000000
test_this "50i",  20000000
test_this "30ms", 30000000
test_this "60ms", 60000000
test_this nil,    20000000
