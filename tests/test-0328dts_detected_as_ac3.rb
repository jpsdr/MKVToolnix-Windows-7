#!/usr/bin/ruby -w

class T_0328dts_detected_as_ac3 < Test
  def description
    "mkvmerge / DTS detected as AC-3"
  end

  def run
    merge "data/dts/misdetected-as-ac3.dts"
    hash_tmp
  end
end
