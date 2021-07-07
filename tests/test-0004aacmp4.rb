#!/usr/bin/ruby -w

class T_0004aacmp4 < Test
  def description
    return "mkvmerge / audio only / in(AAC in MP4)"
  end

  def run
    merge("data/mp4/v.mp4")
    return hash_tmp
  end
end
