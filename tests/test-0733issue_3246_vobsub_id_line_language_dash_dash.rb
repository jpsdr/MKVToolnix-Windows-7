#!/usr/bin/ruby -w

# T_733issue_3246_vobsub_id_line_language_dash_dash
describe "mkvmerge / issue 3246: VobSubs, 'id: --' line indicating language is unknown"
test_merge "data/vobsub/language_unknown.idx"
