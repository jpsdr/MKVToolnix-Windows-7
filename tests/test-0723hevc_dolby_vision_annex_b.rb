#!/usr/bin/ruby -w

# T_723hevc_dolby_vision_annex_b
describe "mkvmerge / HEVC with Dolby Vision in Annex B bitstreams"

%w{dolby-vision-amaze-dolby-vision-www.demolandia.net-5mb.h265
   dv-profile5.h265
   hevc-dolby_vision-hfr-5mb.h265
   iOS_P5_GlassBlowing2_1920x1080@59.94fps_15200kbps-5mb.h265
   P81_GlassBlowing2_1920x1080@59.94fps_15200kbps_fmp4-5mb.h265}.each do |file_name|
  test_merge "data/h265/dolby-vision/#{file_name}"
end
