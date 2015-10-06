#!/usr/bin/ruby -w

describe "mkvmerge / identification of DTS 96/24"
%w{dts-96-24.dts dts-96-24.mka dts-96-24.vob}.each { |file| test_identify "data/dts/#{file}" }
