#!/usr/bin/ruby -w

# T_505cisco_talos_can_0036
describe "mkvmerge, mkvinfo / invalid memory access reported as Cisco TALOS-CAN-0036"

files = Dir.glob("data/segfaults-assertions/cisco-talos-can-0036/*").sort

files.each do |file|
  test_merge file, :exit_code => :warning
  test_info  file, :exit_code => :warning
end
