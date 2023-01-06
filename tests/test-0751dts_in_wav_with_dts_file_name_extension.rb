#!/usr/bin/ruby -w

src = "data/wav/wav_with_dts_extension.dts"

# T_751dts_in_wav_with_dts_file_name_extension
describe "mkvmerge / WAV with DTS inside but with file name extension 'dts'"

test "identification" do
  %r{wav}i.match(identify_json(src)["container"]["type"]) ? "ok" : fail
end

test_merge src
