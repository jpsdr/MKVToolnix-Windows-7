#!/usr/bin/ruby -w

# T_731hevc_es_wrong_extension_detected_as_dts
describe "mkvmerge / HEVC ES with wrong file name extension mis-detected as DTS"

file = "data/h265/issue-3201-hevc-es-wrong-extension-detected-as-dts.mkv"

test_identify file
test_merge file
