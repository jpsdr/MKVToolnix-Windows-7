#!/usr/bin/perl

use strict;
use warnings;

use English;

use JSON::PP;

my $exe = $ENV{MKVMERGE} // 'mkvmerge';

sub mpls_files {
  return grep { (-f $_) && m{mpls$}i } (<*>);
}

sub identify {
  my ($file_name) = @_;

  my $output = `$exe -J $file_name 2> /dev/null`;
  return undef if $CHILD_ERROR != 0;

  $output        = JSON::PP::decode_json($output);
  my ($duration) = $output->{container}->{properties}->{playlist_duration};
  my ($size)     = $output->{container}->{properties}->{playlist_size};

  return undef unless $duration && $size;

  $duration = int($duration / 1_000_000_000);
  $size     = int($size     / 1_000_000);

  return sprintf('%s duration %02d:%02d:%02d size %d MB', $file_name, int($duration / 60 / 60), int($duration / 60) % 60, $duration % 60, $size);
}

print join("\n", grep { $_ } map { identify($_) } mpls_files()), "\n";
