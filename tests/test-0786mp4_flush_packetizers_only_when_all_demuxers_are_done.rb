#!/usr/bin/ruby -w

# T_786mp4_flush_packetizers_only_when_all_demuxers_are_done
describe "mkvmerge / MP4 reader: only flush packetizers when all demuxers have finished processing"

[ "", "-mpeg2" ].each do |infix|
  name = "data/mp4/issue6320-repro#{infix}.mp4"

  test_merge name, :keep_tmp => true

  test "number of frames after muxing #{name}" do
    num_frames = info("-s #{tmp}", :output => :return).
      first.
      select { |line| %r{track 1}.match(line) }.
      size

    num_frames == 240 ? "OK" : "failed"
  end
end
