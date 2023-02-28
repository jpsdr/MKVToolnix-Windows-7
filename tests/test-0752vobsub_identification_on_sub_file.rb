#!/usr/bin/ruby -w

file_base = "data/vobsub/vobsub.not.mpeg.ps."

# T_752vobsub_identification_on_sub_file
describe "mkvmerge / VobSub identification if passed .sub instead of .idx"

test_merge "#{file_base}idx"
test_merge "#{file_base}sub"
