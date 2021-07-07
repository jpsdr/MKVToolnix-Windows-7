#!/usr/bin/ruby -w

class T_0251vc1_truehd_eac3_from_evo < Test
  def description
    return "mkvmerge / VC-1, TrueHD, E-AC-3 from EVO / in(EVO)"
  end

  def run
    merge("data/vob/vc1_truehd_eac3.evo")
    return hash_tmp
  end
end
