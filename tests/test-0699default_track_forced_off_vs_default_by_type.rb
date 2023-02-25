#!/usr/bin/ruby -w

# T_699default_track_forced_off_vs_default_by_type
describe "mkvmerge / Matroska file with 'default track' set, overwritten on command line, track from additional SRT file should become default"

file_names = "data/mkv/sub-is-default.mks data/subtitles/srt/ven.srt"
specs      = [ [ "", true, false ], [ "--default-track-flag 0:yes", true, false ], [ "--default-track-flag 0:no", false, true ] ]

specs.each do |spec|
  test_merge file_names, :args => spec[0], :keep_tmp => true

  test "#{spec}" do
    json = identify_json(tmp)
    (json["tracks"][0]["properties"]["default_track"] == spec[1]) && (json["tracks"][1]["properties"]["default_track"] == spec[2])
  end
end
