#!/usr/bin/ruby -w

class T_0034ac3misdetected_as_mp2 < Test
  def description
    return "mkvmerge / AC-3 misdetected as MP2 / in(AC-3)"
  end

  def run
    merge("data/ac3/misdetected_as_mp2.ac3")
    return hash_tmp
  end
end
