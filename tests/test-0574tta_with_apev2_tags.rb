#!/usr/bin/ruby -w

file = "data/tta/apev2-tags.tta"

# T_574tta_with_apev2_tags
describe "mkvmerge / TTA files with APEv2 tags"
test_identify file
test_merge file
