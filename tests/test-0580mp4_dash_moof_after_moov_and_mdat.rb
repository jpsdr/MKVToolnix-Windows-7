#!/usr/bin/ruby -w

describe "mkvmerge / MP4 DASH with first 'moof' atom located behind first 'moov' and 'mdat' atoms"
test_merge "data/mp4/moof_after_moov_and_mdat.mp4"
