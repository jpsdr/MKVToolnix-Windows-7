#!/usr/bin/ruby -w

# T_546teletext_multiple_pages_in_single_track
describe "mkvmerge / MPEG TS teletext track with multiple pages"
test_merge "data/ts/two-teletext-pages-in-single-ts-track.ts", args: "--no-audio --no-video"
test_merge "data/ts/two-teletext-pages-in-single-ts-track.ts", args: "--no-audio --no-video --subtitle-tracks 2"
test_merge "data/ts/two-teletext-pages-in-single-ts-track.ts", args: "--no-audio --no-video --subtitle-tracks 3"
