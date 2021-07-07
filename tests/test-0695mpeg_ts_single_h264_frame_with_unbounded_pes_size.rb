#!/usr/bin/ruby -w

# T_695mpeg_ts_single_h264_frame_with_unbounded_size
describe "mkvmerge / MPEG TS with a single h.264/AVC frame with an unbounded PES size"

test_merge "data/ts/single_h264_frame_with_unbounded_pes_size.ts"
