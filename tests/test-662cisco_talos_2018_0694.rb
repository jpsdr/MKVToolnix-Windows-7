#!/usr/bin/ruby -w

# T_662cisco_talos_2018_0694
describe "mkvinfo / Cisco TALOS 2018-0694 use after free"

file = "data/segfaults-assertions/cisco-talos-2018-0694.mkv"

test_merge file
