#!/usr/bin/ruby -w

# T_371doc_and_read_version
describe "mkvmerge / DocTypeVersion and DocTypeReadVersion"

test "all versions" do
  sources = "data/avi/v.avi data/subtitles/srt/ven.srt"
  hashes  = []
  hacks   = []

  [ [ nil, 7 ], [ 'no_cue_duration', 7 ], [ 'no_cue_relative_position', 7 ], [ nil, 7 ], [], [ 'no_simpleblocks', 7 ], [ nil, 7 ], [] ].each do |spec|
    hacks   << spec.first if spec.first
    command  = []
    command << ('--engage ' + hacks.join(',')) if !hacks.empty?
    command << "--stereo-mode 0:#{spec.last}"  if spec.last
    command << sources

    command = command.join ' '

    merge command

    type_version, type_read_version = get_doc_type_versions(tmp)

    hashes << "#{type_version}+#{type_read_version}"
  end

  hashes.join('-')
end
