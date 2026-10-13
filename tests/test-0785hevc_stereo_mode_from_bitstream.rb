#!/usr/bin/ruby -w

# T_785hevc_stereo_mode_from_bitstream
describe "mkvmerge / HEVC/H.265, detecting stereo mode from bitstream"

## identification
base_dir = "data/h265/stereo-mode"
files = [
  [ "base-sbs", nil ],
  [ "base-tb", nil ],
  [ "sbs-left", 1 ],
  [ "sbs-right", 11 ],
  [ "tb-left", 3 ],
  [ "tb-right", 2 ],
]
extensions = [ ".h265", ".ts", "-stock.mkv" ]

extensions.each do |ext|
  files.each do |spec|
    file_name = "#{base_dir}/hevc-#{spec[0]}#{ext}"
    expected  = spec[1]

    if FileTest.exist?(file_name)
      test "file #{file_name} expected stereo mode #{expected}" do
        identify_json(file_name)["tracks"][0]["properties"]["stereo_mode"] == expected ? "OK" : "failed"
      end
    end
  end
end

## remux

extensions.each do |ext|
  file_name = "#{base_dir}/hevc-sbs-right#{ext}"

  test_merge file_name, args: "--stereo-mode 0:2", keep_tmp: true
  test "file #{file_name} overwriting stereo mode from 11 to 2" do
    identify_json(tmp)["tracks"][0]["properties"]["stereo_mode"] == 2 ? "OK" : "failed"
  end
end
