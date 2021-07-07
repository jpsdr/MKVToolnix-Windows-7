#!/usr/bin/ruby -w

# T_329X_timecodes_v2
describe "mkvextract / timecodes_v2"

test "timestamp extraction" do
  (0..9).collect do |track_id|
    sys "../src/mkvextract timecodes_v2 data/mkv/complex.mkv #{track_id}:#{tmp}"
    a = hash_tmp
    sys "../src/mkvextract timestamps_v2 data/mkv/complex.mkv #{track_id}:#{tmp}"
    b = hash_tmp

    fail "timecodes_v2 <> timestamps_v2" if a != b

    a
  end.join('-')
end
