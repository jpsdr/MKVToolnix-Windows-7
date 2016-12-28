#!/usr/bin/ruby -w

# T_506cisco_talos_can_0037
describe "mkvmerge, mkvinfo / invalid memory access reported as Cisco TALOS-CAN-0037"

exit_codes = {
  "data/segfaults-assertions/cisco-talos-can-0037/id:000409,sig:06,src:001692,op:havoc,rep:32" => :warning,
}

files = Dir.glob("data/segfaults-assertions/cisco-talos-can-0037/*").sort

files.each { |file| test_info(file, :exit_code => exit_codes[file] || :success) }
