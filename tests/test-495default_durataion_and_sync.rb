#!/usr/bin/ruby -w

# T_495default_durataion_and_sync
src = "data/mp4/test_2000_inloop.mp4"

describe "mkvmerge / --default-duration and --sync at the same time"
test_merge src, :args => "--default-duration 0:25fps"
test_merge src, :args => "--default-duration 0:25fps --sync 0:2000"
test_merge src, :args =>                            "--sync 0:2000"
