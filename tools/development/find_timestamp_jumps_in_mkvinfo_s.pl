#!/usr/bin/perl

use strict;

my ($prev_ts, $prev_line) = (0, '');
my $line_no = 0;

while (<>) {
  $line_no++;
  chomp;

  if (m{timestamp (\d+):(\d+):(\d+)\.(\d+)}) {
    my $curr_ts = $4 + ($3 + $2 * 60 + $1 * 3_600) * 1_000_000_000;

    if (($curr_ts - $prev_ts) > 10_000_000_000) {
      print "--- jump in line $line_no\n$prev_line\n$_\n";
    }

    $prev_ts = $curr_ts;
  }

  $prev_line = $_;
}
