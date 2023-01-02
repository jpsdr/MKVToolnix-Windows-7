#!/usr/bin/ruby -w
file = "data/avi/v-h264-aac.avi"

# T_533chapter_generation_interval
describe "mkvmerge / generate chapter »interval«"

def hash_results max
  ( (1..max).collect { |i| hash_file(sprintf("%s-%02d", tmp, i)) } + [ FileTest.exist?(sprintf("%s-%02d", tmp, max + 1)) ? 'bad' : 'ok' ]).join '+'
end

test_merge "#{file} + #{file} + #{file}", :args => "--generate-chapters interval:30s"
test_merge "#{file} + #{file} + #{file}", :args => "--chapter-language ger --generate-chapters interval:30s"
test "creation and splitting" do
  merge "--chapter-language ger --generate-chapters interval:30s --split 30s #{file} + #{file} + #{file}", :output => "#{tmp}-%02d"
  hash_results 7
end
test_merge "#{file}", :args => "--generate-chapters interval:30s"
