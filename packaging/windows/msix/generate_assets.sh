#!/bin/zsh

set -e
setopt nullglob

# Magick calls Inkscape for converting SVGs if the latter is installedd.
#
# SELF_CALL environment variable is required due to a bug in Inkscape trying to detect if another instance is running.
# If multiple instances start simultaneously, this causes crashes. See https://gitlab.com/inkscape/inkscape/-/work_items/4716
export SELF_CALL=0

function run_magick {
  # ** (inkscape:1672090): WARNING **: 10:00:15.759: Failed to wrap object of type 'PangoFT2FontMap'. Hint: this error is commonly caused by failing to call a library init() function.
  # ** (inkscape:1672090): WARNING **: 10:00:15.817: Failed to wrap object of type 'GtkRecentManager'. Hint: this error is commonly caused by failing to call a library init() function.

  magick $@ |& grep -Eiv 'Hint: this error is commonly caused by failing to call a library init|^$' || true
  return ${pipestatus[1]}
}

rm -rf ${0:a:h}/assets
mkdir -p ${0:a:h}/assets
cd ${0:a:h}/assets

src=../../../../share/icons/mkvtoolnix/mkvtoolnix.svgz

for height in 16 24 32 48 64 256; do
  run_magick -background transparent ${src} -scale ${height}x${height} fileicon.targetsize-${height}.png
done

for scale in 100 125 150 200 400; do
  height=$(( ((scale * 10 * 150) / 100 + 5) / 10 ))
  run_magick -background transparent ${src} -scale ${height}x${height} Square150x150Logo.scale-${scale}.png
done

for scale in 100 125 150 200 400; do
  height=$(( ((scale * 10 * 310) / 100 + 5) / 10 ))
  run_magick -background transparent ${src} -scale ${height}x${height} Square310x310Logo.scale-${scale}.png
done

for scale in 100 125 150 200 400; do
  height=$(( ((scale * 10 * 44) / 100 + 5) / 10 ))
  run_magick -background transparent ${src} -scale ${height}x${height} Square44x44Logo.scale-${scale}.png
done

for height in 16 24 32 48 256; do
  run_magick -background transparent ${src} -scale ${height}x${height} Square44x44Logo.targetsize-${height}.png
done

for scale in 100 125 150 200 400; do
  height=$(( ((scale * 10 * 71) / 100 + 5) / 10 ))
  run_magick -background transparent ${src} -scale ${height}x${height} Square71x71Logo.scale-${scale}.png
done

for scale in 100 125 150 200 400; do
  height=$(( ((scale * 10) / 2 + 5) / 10 ))
  run_magick -background transparent ${src} -scale ${height}x${height} StoreLogo.scale-${scale}.png
done

for scale in 100 125 150 200 400; do
  height=$(( ((scale * 10 * 150) / 100 + 5) / 10 ))
  width=$(( ((scale * 10 * 310) / 100 + 5) / 10 ))
  run_magick -background transparent ${src} -scale ${height}x${height} -gravity center -extent ${width}x${height} Wide310x150Logo.scale-${scale}.png
done

for file in *.scale-100.png; do
  cp ${file} ${file/.scale-100/}
done
