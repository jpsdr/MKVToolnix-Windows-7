#!/usr/bin/ruby -w

# T_633payload_signalled_but_no_payload_bytes
describe "mkvmerge / MPEG transport stream with packets signalling TS payload but with no payload bytes"
test_merge "data/ts/payload_signalled_but_no_payload_bytes.ts"
