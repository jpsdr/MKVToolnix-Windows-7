#!/usr/bin/ruby -w

# T_665X_aac_program_config_element_in_audio_specific_config
describe "mkvextract / AAC with program_config_elements in AudioSpecificConfig and no channel information"

base = "data/aac/program_config_element_in_audio_specific_config_"

(1..2).each do |idx|
  file = "#{base}#{idx}.mka"

  test "extract from #{file}" do
    extract file, 0 => tmp
    hash_tmp
  end
end
