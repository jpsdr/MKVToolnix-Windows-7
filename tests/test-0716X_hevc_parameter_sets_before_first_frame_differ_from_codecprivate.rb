#!/usr/bin/ruby -w

# T_716X_hevc_parameter_sets_before_first_frame_differ_from_codecprivate
describe "mkvextract data/mkv/hevc_parameter_sets_before_first_frame_differ_from_codecprivate.mkv"

test "extraction" do
  extract "data/mkv/hevc_parameter_sets_before_first_frame_differ_from_codecprivate.mkv", 0 => tmp
  hash_file tmp
end
