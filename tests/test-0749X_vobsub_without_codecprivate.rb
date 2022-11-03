#!/usr/bin/ruby -w

# T_749X_vobsub_without_codecprivate
describe "mkvextract VobSub without CodecPrivate"

[0, 1].each do |track_id|
  test "extraction track ID #{track_id}" do
    extract "data/vobsub/vobsub-without-codecprivate.mkv", track_id => "#{tmp}.idx"

    hash_file("#{tmp}.idx") + "+" + hash_file("#{tmp}.sub")
  end
end

test "extraction all tracks to same file" do
  extract "data/vobsub/vobsub-without-codecprivate.mkv", 0 => "#{tmp}.idx", 1 => "#{tmp}.idx"

  hash_file("#{tmp}.idx") + "+" + hash_file("#{tmp}.sub")
end
