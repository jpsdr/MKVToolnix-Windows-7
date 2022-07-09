#!/usr/bin/ruby -w

# T_743propedit_video_colour_attributes_old_names
describe "mkvpropedit / video colour attributes (old spelling)"

def args *arrays
  arrays.flatten.map.with_index { |elt, idx| "--set #{elt}=#{idx + 1}" }.join(" ")
end

test "set all attributes" do
  merge "--no-audio data/avi/v.avi", :no_result => true
  src = tmp

  colour_attributes = [
    "colour-matrix-coefficients",
    "colour-bits-per-channel",
    "chroma-subsample-horizontal",
    "chroma-subsample-vertical",
    "cb-subsample-horizontal",
    "cb-subsample-vertical",
    "chroma-siting-horizontal",
    "chroma-siting-vertical",
    "colour-range",
    "colour-transfer-characteristics",
    "colour-primaries",
    "max-content-light",
    "max-frame-light",
  ]

  colour_mastering_meta_attributes = [
    "chromaticity-coordinates-red-x",
    "chromaticity-coordinates-red-y",
    "chromaticity-coordinates-green-x",
    "chromaticity-coordinates-green-y",
    "chromaticity-coordinates-blue-x",
    "chromaticity-coordinates-blue-y",
    "white-coordinates-x",
    "white-coordinates-y",
    "max-luminance",
    "min-luminance",
  ]

  hashes = []
  work   = "#{src}-1"

  cp src, work
  propedit work, "--edit track:v1 --set pixel-width=1024"
  hashes << hash_file(work)

  cp src, work
  propedit work, "--edit track:v1 --set pixel-width=1024 #{args(colour_attributes)}"
  hashes << hash_file(work)

  cp src, work
  propedit work, "--edit track:v1 --set pixel-width=1024 #{args(colour_mastering_meta_attributes)}"
  hashes << hash_file(work)

  cp src, work
  propedit work, "--edit track:v1 --set pixel-width=1024 #{args(colour_attributes, colour_mastering_meta_attributes)}"
  hashes << hash_file(work)

  hashes.join("+")
end

test "delete attributes" do
  merge "--no-audio --max-frame-light 0:41 --min-luminance 0:42 data/avi/v.avi", :no_result => true

  hashes = []
  src    = tmp
  work   = "#{src}-1"

  cp src, work
  propedit work, "--edit track:v1 --delete language"
  hashes << hash_file(work)
  # sys "mkvinfo #{work} >&2"

  cp src, work
  propedit work, "--edit track:v1 --delete language --delete min-luminance"
  hashes << hash_file(work)
  # sys "mkvinfo #{work} >&2"

  cp src, work
  propedit work, "--edit track:v1 --delete language --delete max-frame-light"
  hashes << hash_file(work)
  # sys "mkvinfo #{work} >&2"

  cp src, work
  propedit work, "--edit track:v1 --delete language --delete max-frame-light --delete min-luminance"
  hashes << hash_file(work)
  # sys "mkvinfo #{work} >&2"

  hashes.join("+")
end
