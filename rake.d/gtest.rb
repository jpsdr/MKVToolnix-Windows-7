#!/usr/bin/env ruby

$gtest_apps     = %w{common gui merge propedit}
$gtest_internal = c(:GTEST_TYPE) == "internal"

namespace :tests do
  desc "Build the unit tests"
  task :unit => $gtest_apps.collect { |app| "tests/unit/#{app}/#{app}" + c(:EXEEXT) }

  desc "Build and run the unit tests"
  task :run_unit => 'tests:unit' do
    runtime, args = $building_for[:windows] ? [ "wine", "--gtest_color=no | tr -d '\n'" ] : [ "", "" ]
    $gtest_apps.each { |app| run "LC_ALL=C #{runtime} ./tests/unit/#{app}/#{app}#{c(:EXEEXT)} #{args}" }
  end

  if !c(:VALGRIND).to_s.empty?
    desc "Build and run the unit tests under valgrind's memcheck"
    task :run_valgrind_unit => 'tests:unit' do
      $gtest_apps.each { |app| run "LC_ALL=C #{c(:VALGRIND)} --tool=memcheck --num-callers=12 ./tests/unit/#{app}/#{app}" }
    end
  end
end

$build_system_modules[:gtest] = {
  :setup => lambda do
    if $gtest_internal
      $flags[:cxxflags] += " -Ilib/gtest -Ilib/gtest/include"
    end
  end,

  :define_tasks => lambda do
    gtest_libs = {
      'propedit' => [ :mtxpropedit ],
      'merge'    => [ :mtxmerge ],
    }

    gtest_libs.default = []

    gtest_gui_sources = {
      'gui' => [ "src/mkvtoolnix-gui/util/json.cpp", "src/mkvtoolnix-gui/util/string.cpp", ],
    }

    conditions = {
      'gui' => $build_mkvtoolnix_gui,
    }

    gtest_gui_sources.default = []

    #
    # Google Test framework
    #
    if $gtest_internal
      Library.
        new('lib/gtest/src/libgtest').
        sources([ 'lib/gtest/src' ], :type => :dir, :except => [ 'gtest-all.cc' ]).
        create
    end

    Library.
      new('tests/unit/libmtxunittest').
      sources('tests/unit', :type => :dir).
      create

    $gtest_apps.each do |app|
      next if !conditions[app]

      Application.
        new("tests/unit/#{app}/#{app}").
        description("Build the unit tests executable for '#{app}'").
        aliases("unit_tests_#{app}").
        sources([ "tests/unit/#{app}" ], :type => :dir).
        sources(gtest_gui_sources[app]).
        libraries(gtest_libs[app], :mtxunittest, $common_libs, :gtest, :pthread).
        create
    end
  end,
}
