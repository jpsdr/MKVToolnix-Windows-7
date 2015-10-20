#!/usr/bin/ruby -w

# T_505cisco_talos_can_0036
describe "mkvmerge, mkvinfo / invalid memory access reported as Cisco TALOS-CAN-0036"

files = Dir.glob("data/segfaults-assertions/cisco-talos-can-0036/*")

files.each do |file|
  test_merge file
  test_info  file
end
