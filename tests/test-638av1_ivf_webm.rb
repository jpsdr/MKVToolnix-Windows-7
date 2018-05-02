#!/usr/bin/ruby -w

# T_638av1_ivf_webm
describe "mkvmerge / reading AV1 from IVF, WebM; writing Matroska, WebM"

base = "data/av1/av1."

test_merge "#{base}ivf",  :args => "--engage enable_av1"
test_merge "#{base}webm", :args => "--engage enable_av1"

test_merge "#{base}ivf",  :args => "--engage enable_av1 --webm"
test_merge "#{base}webm", :args => "--engage enable_av1 --webm"
