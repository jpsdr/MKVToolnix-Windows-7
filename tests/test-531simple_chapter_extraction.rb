#!/usr/bin/ruby -w

# T_531simple_chapter_extraction
describe "mkvextract / chapters in simple format including language selection"

%w{love phantom}.each do |file|
  [ "", "--simple-language eng", "--simple-language spa" ].each do |language|
    test "extract from #{file} #{language}" do
      extract "data/chapters/#{file}.mkv", :args => "--simple #{language} #{tmp}", :mode => :chapters
      hash_tmp
    end
  end
end
