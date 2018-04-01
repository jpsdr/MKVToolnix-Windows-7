#!/usr/bin/ruby -w

# T_638av1_ivf_webm
describe "mkvmerge / reading AV1 from IVF, WebM; writing Matroska, WebM"

base = "data/av1/av1."

test_merge "#{base}ivf"
test_merge "#{base}webm"

test_merge "#{base}ivf",  :args => "--webm"
test_merge "#{base}webm", :args => "--webm"
