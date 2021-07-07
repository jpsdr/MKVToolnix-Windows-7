#!/usr/bin/ruby -w

class T_0288identify_files_by_amg < Test
  def description
    "mkvmerge / identify on files created by AMG"
  end

  def run
    content = JSON.load(capture_bash("../src/mkvmerge -J data/mkv/amg_sample.mkv"))["tracks"].size.to_s
    IO.write(tmp, content + "\n", mode: 'wb')
    hash_tmp
  end
end
