#!/usr/bin/ruby -w

# T_033timecode_scale
describe "mkvmerge / timecode scale / in(AVI,MP3)"

test "timecode/timestamp scale" do
  hash = []

  merge("--timecode-scale 1000000 data/simple/v.mp3")
  hash << hash_tmp
  merge("--timecode-scale -1 data/avi/v.avi")
  hash << hash_tmp

  merge("--timestamp-scale 1000000 data/simple/v.mp3")
  hash << hash_tmp
  merge("--timestamp-scale -1 data/avi/v.avi")
  hash << hash_tmp

  hash.join('-')
end
