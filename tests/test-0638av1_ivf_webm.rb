#!/usr/bin/ruby -w

# T_638av1_ivf_webm
describe "mkvmerge / reading AV1 from IVF, WebM, OBU; writing Matroska, WebM"

base = "data/av1/av1."

[ "", "--webm" ].each do |args|
  [ "ivf", "webm", "obu" ].each do |ext|
    test_merge "#{base}#{ext}", :args => args
  end
end
