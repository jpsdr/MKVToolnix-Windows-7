#!/usr/bin/ruby -w

# T_684deterministic
describe "mkvmerge / deterministic option"

def doit(seed)
  merge "--deterministic #{seed} data/mp4/3covers.mp4", :no_variable_data => false, :no_result => true

  json = identify_json(tmp)
  uids = [
    json["container"]["properties"]["segment_uid"],
    json["attachments"].map { |a| a["properties"]["uid"] },
    json["tracks"]     .map { |t| t["properties"]["uid"] },
  ].flatten.map(&:to_s).join('+')

  clean_tmp

  return uids
end

test("SomeSeed 1") { doit("SomeSeed") }
test("SomeSeed 2") { doit("SomeSeed") }

test("AnotherSeed 1") { doit("AnotherSeed") }
test("AnotherSeed 2") { doit("AnotherSeed") }
