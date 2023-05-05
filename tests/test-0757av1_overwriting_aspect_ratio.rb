#!/usr/bin/ruby -w

# T_757av1_overwriting_aspect_ratio
describe "mkvmerge / overwriting the aspect ratio of AV1 tracks"

def get_display_dimensions file_name
  identify_json(file_name)["tracks"][0]["properties"]["display_dimensions"]
end

def run_test_with_args initial_args
  merge "#{initial_args} -A data/av1/av1.ivf"
  result = get_display_dimensions tmp
  [ "", "--display-dimensions 0:1212x2424", "--aspect-ratio 0:5", "--aspect-ratio-factor 0:5" ].each_with_index do |args, idx|
    merge "#{args} #{tmp}", :output => "#{tmp}#{idx}"
    result += "-" + get_display_dimensions("#{tmp}#{idx}")
  end

  result
end

[ "--display-dimensions 0:4254x815", "--aspect-ratio 0:10", "--aspect-ratio-factor 0:10" ].each_with_index do |args, idx|
  test "initial args #{args}" do
    "#{idx}[" + run_test_with_args(args) + "]"
  end
end
