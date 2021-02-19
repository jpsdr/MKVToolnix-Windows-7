#!/usr/bin/ruby -w

# T_718non_ascii_characters_in_file_names
describe "mkvmerge & mkvextract / non-ASCII characters in file names"

Dir.glob("data/filenames/*").sort.each do |file_name|
  test_identify file_name
  test_merge    file_name

  if %r{mks$}.match(file_name)
    test "extraction of #{file_name}" do
      extract file_name, 0 => tmp
      hash_tmp
    end
  end
end
