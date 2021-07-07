#!/usr/bin/ruby -w

# T_509rerender_track_headers_chapters_attachments
describe "mkvmerge / rerender_track_headers with chapters and attachments"

dir = "data/mkv/rerender_track_headers_chapters_attachments"
test_merge "#{dir}/sample.mkv", :args => "#{dir}/sample_video.hevc -T --no-chapters --no-global-tags #{dir}/sample_audio.opus -D -A -T"
