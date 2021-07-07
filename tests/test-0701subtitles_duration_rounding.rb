#!/usr/bin/ruby -w

# T_701subtitles_duration_rounding
describe "mkvmerge / subtitles: taking timestamp rounding errors into account when calculating the rounded duration"
test_merge "data/subtitles/srt/duration_rounding.srt"
