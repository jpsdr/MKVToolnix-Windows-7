#!/usr/bin/ruby -w

# T_711av1_last_frame_duration_when_appending
describe "mkvmerge / AV1 frame durations & appending"

%w{ivf obu}.each do |ext|
  test_merge((1..4).map { |idx| "data/av1/parts/v-00#{idx}.#{ext}" }.join(' + '))
end
