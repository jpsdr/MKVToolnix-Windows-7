#!/usr/bin/ruby -w

class T_0209ac3misdeetected_as_mpeges < Test
  def description
    return "mkvmerge / AC-3 misdetected as MPEG-ES / in(AC-3)"
  end

  def run
    merge("data/ac3/misdetected_as_mpeges.ac3")
    return hash_tmp
  end
end
