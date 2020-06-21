#!/usr/bin/perl

use strict;
use warnings;

sub fmt {
  my ($orig, $direction, $ts) = @_;

  my $seconds = int($ts / 1_000_000_000);

  printf "%d %s MPEG is %02d:%02d:%02d.%09d\n", $orig, $direction, int($seconds / 3_600), int($seconds / 60) % 60, $seconds % 60, $ts % 1_000_000_000;
}

my $direction = 'from';

foreach my $arg (@ARGV) {
  if ($arg =~ m{^(f|from)$}) {
    $direction = 'from';

  } if ($arg =~ m{^(t|to)$}) {
    $direction = 'to';

  } elsif ($direction eq 'from') {
    fmt($arg, $direction, int($arg * 100_000 / 9));

  } else {
    fmt($arg, $direction, int($arg * 9 / 100_000));
  }
}
