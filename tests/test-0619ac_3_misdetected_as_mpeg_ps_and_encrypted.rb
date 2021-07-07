#!/usr/bin/ruby -w

# T_619ac_3_misdetected_as_mpeg_ps_and_encrypted
describe "mkvmerge / AC-3 mis-detected as an encrypted MPEG program stream"
test_identify "data/ac3/misdetected_as_mpegps_and_encrypted.ac3"
