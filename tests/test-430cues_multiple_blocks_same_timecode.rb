#!/usr/bin/ruby -w

describe "mkvmerge / Cues for tracks with multiple blocks at the same timecode"
test_merge "data/subtitles/srt/two-at-same-timecode.srt"
