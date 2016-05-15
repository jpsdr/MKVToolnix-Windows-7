#!/usr/bin/ruby -w

# T_548generate_chapters_interval
describe "mkvmerge / generate chapters in interval, exceed void size"

[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60, 80].each do |interval|
  test_merge "data/simple/v.mp3", args: "--generate-chapters interval:#{interval}s", keep_tmp: true
  test "number of chapters" do
    identify_json(tmp)["chapters"][0]["num_entries"]
  end
end
