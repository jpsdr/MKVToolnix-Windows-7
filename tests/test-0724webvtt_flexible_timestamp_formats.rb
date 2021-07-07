#!/usr/bin/ruby -w

# T_724webvtt_flexible_timestamp_formats
describe "mkvmerge / WebVTT, more flexible parsing of timestamps"

test_merge "data/subtitles/webvtt/flexible_timestamp_formats.vtt"
