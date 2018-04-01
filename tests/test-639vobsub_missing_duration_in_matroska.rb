#!/usr/bin/ruby -w

# T_639vobsub_missing_duration_in_matroska
describe "mkvmerge / VobSub packets without a duration on the container level"
test_merge "data/vobsub/no_duration_in_matroska.mkv", :args => "-A -D -s 4"
