#!/usr/bin/perl -w

use strict;
use YAML::Tiny;


my $data = YAML::Tiny::LoadFile($ARGV[0])
  or die "Can't read $ARGV[0]: $!\n";

my $pwd='1234a';

foreach my $k (keys $data){
  my $owner = $data->{$k}->{owner} || '';
  my $ctime = $data->{$k}->{ctime} || '';
  my $body  = $data->{$k}->{body} || '';
  my $people = $data->{$k}->{people} || '';
  my $route  = $data->{$k}->{route} || '';
  my $title  = $data->{$k}->{title} || '';
  my $date1  = $data->{$k}->{date1} || '';
  my $date2  = $data->{$k}->{date2} || '';
  my $tags   = $data->{$k}->{tags} || '';

  my @t1;
  foreach (@{$tags}){ push (@t1, $tag_nums{$_}) if exists $tag_nums{$_};}
  $tags = join ',', @t1;

  open IN, "../eventdb '$owner' '' event_create " .
           "'$title' '$body' '$people' '$route' " .
           "'$date1' '$date2' '$tags' |" or die "Can't run eventdb\n";
  my $ret = <IN>;
  close $ret;
  chomp($ret);

  print "$k -> $owner -> $tags\n";
}
