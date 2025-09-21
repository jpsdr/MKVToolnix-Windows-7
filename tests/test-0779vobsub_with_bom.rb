#!/usr/bin/ruby -w

# T_779vobsub_with_bom
describe "mkvmerge / VobSub's .idx starts with a UTF BOM"

test_merge "data/vobsub/with_bom.idx"
