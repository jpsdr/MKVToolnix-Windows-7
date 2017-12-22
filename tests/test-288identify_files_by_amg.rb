#!/usr/bin/ruby -w

class T_288identify_files_by_amg < Test
  def description
    "mkvmerge / identify on files created by AMG"
  end

  def run
    File.open(tmp, 'w') do |file|
      file.puts(JSON.load(`../src/mkvmerge -J data/mkv/amg_sample.mkv`)["tracks"].size.to_s)
    end
    hash_tmp
  end
end
