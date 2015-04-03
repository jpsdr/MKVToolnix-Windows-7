#!/usr/bin/ruby -w

# T_489dts_es
describe "mkvmerge / identify DTS-ES"

%w{dts/dts-es.dts ts/dts-es.m2ts vob/dts-es.vob}.each { |file| test_identify "data/#{file}" }
