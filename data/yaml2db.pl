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
  my $date   = $data->{$k}->{date} || '';
  my @tags   = $data->{$k}->{tags} || [];

  my ($y1, $m1, $d1, $y2, $m2, $d2) =(0,0,0,0,0,0);
  if ($date =~ /^(\d{4})[\.\/](\d{2})[\.\/](\d{2})/){($y1,$m1,$d1)=($1,$2,$3);}
  elsif ($date =~ /^(\d{4})[\.\/](\d{2})/){($y1,$m1)=($1,$2);}
  elsif ($date =~ /^(\d{4})/){$y1=$1;}

  if ($date =~ /-(\d{4})[\.\/](\d{2})[\.\/](\d{2})/){($y2,$m2,$d2)=($1,$2,$3);}
  elsif ($date =~ /-(\d{2})[\.\/](\d{2})/){($m2,$d2)=($1,$2);}
  elsif ($date =~ /-(\d{2})/){$d2=$1;}

  $y2=$y1 if $y2==0;
  $m2=$m1 if $m2==0;
  $d2=$d1 if $d2==0;

  my $D1="$y1$m1$d1";
  my $D2="$y2$m2$d2";
  my $tags='123,456';

  open IN, "../eventdb '$owner' '' event_create " .
           "'$title' '$body' '$people' '$route' " .
           "'$D1' '$D2' '$tags' |" or die "Can't run eventdb\n";
  my $ret = <IN>;
  chomp($ret);

  print "$k -> $owner -> $ret\n";
  
}
