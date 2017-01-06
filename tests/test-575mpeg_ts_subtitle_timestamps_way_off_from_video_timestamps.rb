#!/usr/bin/ruby -w

# T_575mpeg_ts_subtitle_timestamps_way_off_from_video_timestamps
describe "mkvmerge / MPEG TS: subtitles multiplexed properly, but timestamps way off from audio & video timestamps"
test_merge "data/ts/teletext_subs_timestamps_way_off_from_video_timestamps.ts"
