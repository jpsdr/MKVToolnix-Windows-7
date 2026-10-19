#!/usr/bin/ruby -w

# T_787mp4_ensure_packetizers_are_actually_flushed
describe "mkvmerge / MP4 reader: ensure all packetizers are actually flushed"

[ "", "-mpeg2" ].each do |infix|
  name = "data/mp4/issue6321-repro2#{infix}.mp4"

  test_merge name, :keep_tmp => true

  test "number of frames after muxing #{name}" do
    num_frames = info("-s #{tmp}", :output => :return).
      first.
      select { |line| %r{track 1}.match(line) }.
      size

    num_frames == 240 ? "OK" : "failed"
  end
end
