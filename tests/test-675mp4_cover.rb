#!/usr/bin/ruby -w

# T_675mp4_cover
describe "mkvmerge / reading cover art from MP4s"
test_merge "data/alac/othertest-itunes.m4a", :args => "--no-attachments"
test_merge "data/alac/othertest-itunes.m4a", :keep_tmp => true
test "checksum" do
  extract tmp, 1 => "#{tmp}-att1", :mode => :attachments

  expected_md5 = "060170d57bac3c83175be67abc289b27"
  actual_md5   = md5("#{tmp}-att1")

  [ expected_md5, actual_md5, expected_md5 == actual_md5 ? "true" : "false" ].join("+")
end

test_merge "data/mp4/3covers.mp4"
