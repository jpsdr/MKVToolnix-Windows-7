#!/usr/bin/ruby -w

# T_498mp2_misidentification
describe "mkvmerge / MP2 mis-identified as MP3"

test_identify "data/mpeg12/mpeg1-misdetected-as-avc.mpg"
