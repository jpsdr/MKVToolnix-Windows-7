#!/usr/bin/ruby -w

# T_540teletext_non_subtitle_pages
describe "mkvmerge / teletext subtitles with non-subtitle pages in between"
test_merge "data/ts/teletext_non_subtitle_pages.ts", args: "--no-audio --no-video"
