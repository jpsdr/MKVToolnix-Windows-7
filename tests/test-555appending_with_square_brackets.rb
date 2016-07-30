#!/usr/bin/ruby -w

s1 = "data/subtitles/srt/vde-utf-8-bom.srt"
s2 = "data/subtitles/srt/ven.srt"
m1 = "data/simple/v.mp3"
m2 = "data/simple/v.mp3"

# T_555appending_with_square_brackets
describe "mkvmerge / appending files with the [ square brackets syntax ]"
test_merge "#{s1} + #{s2} + #{s1} + #{s2} #{m1} + #{m2}"
test_merge "'[' #{s1} #{s2} #{s1} #{s2} ']' #{m1} + #{m2}"
test_merge "#{s1} + '[' #{s2} #{s1} #{s2} ']' #{m1} + #{m2}"
test_merge "'[' #{s1} #{s2} #{s1} ']' + #{s2} #{m1} + #{m2}"
test_merge "'[' #{s1} #{s2} #{s1} #{s2} ']' '[' #{m1} #{m2} ']'"
test_merge "'[' #{s1} #{s2} ']' + '[' #{s1} #{s2} ']' '[' #{m1} #{m2} ']'"
