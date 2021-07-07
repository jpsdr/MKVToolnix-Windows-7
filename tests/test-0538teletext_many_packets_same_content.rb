#!/usr/bin/ruby -w

# T_538teletext_many_packets_same_content
describe "mkvmerge / MPEG TS with teletext subtitles with many packets having the same content"
test_merge "data/ts/teletext_subs_many_packets_same_content.ts", args: "--no-audio --no-video"
