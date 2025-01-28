#!/usr/bin/ruby -w

# T_771truehd_second_sync_frame_after_128kb
describe "mkvmerge / TrueHD with second sync frame after 128 KB"

(1..2).each do |idx|
  file_name = "second_sync_frame_after_128kb_#{idx}.thd"

  test "identify #{file_name}" do
    container = identify_json("data/truehd/#{file_name}")["container"]["type"]
    fail "Actual container format '#{container}' is not expected 'TrueHD'" if container != "TrueHD"
    container
  end
end
