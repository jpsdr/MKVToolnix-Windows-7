#!/usr/bin/ruby -w

class T_0204wavpack_without_correctiondata < Test
  def description
    return "mkvmerge / audio only / WavPack without correction data (*.wv)"
  end

  def run
    merge("data/wp/without-correction.wv")
    return hash_tmp
  end
end

