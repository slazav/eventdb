#!/usr/bin/perl -w

use strict;
use YAML::Tiny;

my $data = YAML::Tiny::LoadFile($ARGV[0])
  or die "Can't read $ARGV[0]: $!\n";

my %tag_nums=(
  pd => 1001,
);

my $pwd='1234a';

foreach my $k (keys $data){
  my $owner = $data->{$k}->{cuser} || '';
  my $ctime = $data->{$k}->{ctime} || '';
  my $body  = $data->{$k}->{body} || '';
  my $people = $data->{$k}->{people} || '';
  my $route  = $data->{$k}->{route} || '';
  my $title  = $data->{$k}->{title} || '';
  my @tags   = $data->{$k}->{tags} || [];


  my $d1=123;
  my $d2=234;
  my $tags='123,456';

  open IN, "../eventdb '$owner' '' event_create " .
           "'$title' '$body' '$people' '$route' " .
           "'$d1' '$d2' '$tags' |" or die "Can't run eventdb\n";
  my $ret = <IN>;
  chomp($ret);

  print "$k -> $owner -> $ret\n";
  
}
