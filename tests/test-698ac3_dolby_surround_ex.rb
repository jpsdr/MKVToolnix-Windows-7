#!/usr/bin/ruby -w

# T_698ac3_dolby_surround_ex
describe "mkvmerge / codec specialization 'AC-3 Dolby Surround EX'"

[ "ac3", "mkv", "vob" ].each do |ext|
  file_name = "data/#{ext}/ac3-dolby-surround-ex.#{ext}"

  test "identify #{file_name}" do
    identify_json(file_name)["tracks"].last["codec"] == "AC-3 Dolby Surround EX"
  end
end
