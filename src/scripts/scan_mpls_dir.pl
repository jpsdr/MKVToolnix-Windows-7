#!/usr/bin/perl

use strict;
use warnings;

use English;

my $exe = $ENV{MKVMERGE} // 'mkvmerge';

sub mpls_files {
  return grep { (-f $_) && m{mpls$}i } (<*>);
}

sub identify {
  my ($file_name) = @_;

  my $output = `$exe --identify-verbose $file_name 2> /dev/null`;
  return undef if $CHILD_ERROR != 0;

  my ($duration) = $output =~ m{playlist_duration:(\d+)};
  my ($size)     = $output =~ m{playlist_size:(\d+)};

  return undef unless $duration && $size;

  $duration = int($duration / 1_000_000_000);
  $size     = int($size     / 1_000_000);

  return sprintf('%s duration %02d:%02d:%02d size %d MB', $file_name, int($duration / 60 / 60), int($duration / 60) % 60, $duration % 60, $size);
}

print join("\n", grep { $_ } map { identify($_) } mpls_files()), "\n";
