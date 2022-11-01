#!/usr/bin/ruby -w

# T_747reversed_attachment_selection
describe "mkvmerge / reversed attachment selection"

file = "data/mkv/attachments.mkv"

test "initial number of attachments" do
  identify_json(file)["attachments"].size == 5 ? "ok" : "bad"
end

test_merge file, :args => "-m '!3,4' -s '!0'", :keep_tmp => true

test "number of attachments after muxing" do
  identify_json(tmp)["attachments"].size == 3 ? "ok" : "bad"
end

test_merge file, :args => "-m '3,4' -s '!0'", :keep_tmp => true

test "number of attachments after muxing" do
  identify_json(tmp)["attachments"].size == 2 ? "ok" : "bad"
end
