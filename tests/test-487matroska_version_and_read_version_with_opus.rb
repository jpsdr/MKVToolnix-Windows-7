#!/usr/bin/ruby -w

# T_487matroska_version_and_read_version_with_opus
describe "mkvmerge / DocTypeVersion and DocTypeReadVersion with Opus"

test "all versions" do
  source = "data/opus/v-opus.ogg"
  hashes = []

  [ nil,
    'no_cue_duration', 'no_cue_relative_position', 'no_simpleblocks',
    'no_cue_duration,no_cue_relative_position', 'no_cue_duration,no_simpleblocks', 'no_cue_relative_position,no_simpleblocks',
    'no_cue_duration,no_cue_relative_position,no_simpleblocks',
  ].each do |hacks|
    command  = []
    command << "--engage #{hacks}" if hacks
    command << source

    command = command.join ' '

    merge command

    type_version, type_read_version = get_doc_type_versions(tmp)

    hashes << "#{type_version}+#{type_read_version}"
  end

  hashes.join('-')
end
