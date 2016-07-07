#!/usr/bin/ruby -w

#!/usr/bin/ruby -w

# T_298ts_language
describe "mkvmerge / MPEG transport streams: language tags"

test "detected tracks" do
  json_summarize_tracks(identify_json("data/ts/mp3_listed_in_pmt_but_no_data.ts"))
end
