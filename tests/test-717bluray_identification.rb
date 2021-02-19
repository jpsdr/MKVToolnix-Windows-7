#!/usr/bin/ruby -w

# T_717bluray_identification
describe "mkvmerge / Blu-ray directory structure identification"

file = "data/bluray/ganster_squad/BDMV/PLAYLIST/00100.mpls"

test_identify file
test_merge    file
