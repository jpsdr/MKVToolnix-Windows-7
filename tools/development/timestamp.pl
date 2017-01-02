#!/usr/bin/perl

use strict;
use warnings;

my $timestamp = shift(@ARGV);
$timestamp   *= shift(@ARGV) if @ARGV;

printf "%02d:%02d:%02d.%09d\n",
  int($timestamp / 60 / 60 / 1000000000),
  int($timestamp / 60 / 1000000000) % 60,
  int($timestamp / 1000000000) % 60,
  $timestamp % 1000000000;
