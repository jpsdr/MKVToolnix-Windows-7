#!/usr/bin/ruby -w

# T_611info_null_pointer_dereference_for_ebmlbinary
describe "mkvinfo / null pointer dereference for EbmlBinary with data pointer set to null pointer (bug 2072)"
test_info "data/bugs/2072.mkv"
