#!/usr/bin/ruby -w

# T_676cover_art_vorbis_opus
describe "mkvmerge / reading cover art from Vorbis & Opus tags"

expected_md5s = {
  :png => "72e193b0682dad985317cd03d2be1ab9",
  :jpg => "e55849ace1748ac515b16881e397101a",
}

[ :png, :jpg ].each do |image_type|
  [ :opus, :ogg ].each do |ext|
    file_name = "data/#{ext}/metadata_block_picture_#{image_type}.#{ext}"

    test_merge file_name, :args => "--no-attachments"
    test_merge file_name, :keep_tmp => true
    test "checksum #{image_type} #{ext}" do
      extract tmp, 1 => "#{tmp}-att1", :mode => :attachments

      actual_md5   = md5("#{tmp}-att1")

      [ expected_md5s[image_type], actual_md5, expected_md5s[image_type] == actual_md5 ? "true" : "false" ].join("+")
    end
  end
end
