#!/usr/bin/ruby -w

# T_506cisco_talos_can_0037
describe "mkvmerge, mkvinfo / invalid memory access reported as Cisco TALOS-CAN-0037"

files = Dir.glob("data/segfaults-assertions/cisco-talos-can-0037/*")

files.each { |file| test_info file }
