#!/usr/bin/ruby -w

the_file = "data/mkv/deprecated-languages.mkv"

# T_378deprecated_iso_639_2_codes
describe "mkvmerge / handling deprecated ISO 639-2 language codes"
test "identification" do
  identify_json(the_file)["tracks"].collect { |t| t["properties"]["language"] }.compact.sort.join('+')
end

test_merge the_file, :exit_code => :warning
