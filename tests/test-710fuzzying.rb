#!/usr/bin/ruby -w

# T_710fuzzying
describe "mkvmerge / issues found by fuzzying"

Dir["data/segfaults-assertions/fuzzying/0001-ac3/**/id*"].each do |file|
  exit_code = file.gsub(%r{.*/([012])/.*}, '\1').to_i

  if exit_code < 2
    test_merge file, :exit_code => exit_code
  else
    test_identify file, :exit_code => exit_code
  end
end
