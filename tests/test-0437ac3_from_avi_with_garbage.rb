#!/usr/bin/ruby -w

# T_437ac3_from_avi_with_garbage
describe "mkvmerge / AC with garbage read from AVIs for audio sync"
test_merge "data/avi/ac3_with_garbage.avi"
