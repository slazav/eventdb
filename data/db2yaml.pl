#!/usr/bin/perl -w

use strict;
use YAML::Tiny;

## note: users md5 can be shown only to the root
my $usr='';
my $pwd='';

my %data;

######################################################################
## Users
open IN, "../eventdb '$usr' '$pwd' user_list |" or die "Can't run eventdb\n";
foreach(<IN>){
  chomp;
  my ($name, $act, $lvl, $md5) = split ':';
  next unless $name;
  $data{users} -> {$name} = {
    act=>$act || '',
    lvl=>$lvl || '',
    md5=>$md5 || '',
  };
}
close IN;

######################################################################
## Events
open IN, "../eventdb '$usr' '$pwd' event_list |" or die "Can't run eventdb\n";
my ($id, %ev);
foreach(<IN>){
  if (m|<event id=(\d+)>|){ $id = $1 || 0; %ev=(); }
  elsif (m|<title>(.*)</title>|)  { $ev{title} = $1 || '';}
  elsif (m|<body>(.*)</body>|)    { $ev{body} = $1 || '';}
  elsif (m|<date1>(.*)</date1>|)  { $ev{date1} = $1 || '';}
  elsif (m|<date2>(.*)</date2>|)  { $ev{date2} = $1 || '';}
  elsif (m|<people>(.*)</people>|){ $ev{people} = $1 || '';}
  elsif (m|<route>(.*)</route>|)  { $ev{route} = $1 || '';}
  elsif (m|<ctime>(.*)</ctime>|)  { $ev{ctime} = $1 || '';}
  elsif (m|<mtime>(.*)</mtime>|)  { $ev{mtime} = $1 || '';}
  elsif (m|<owner>(.*)</owner>|)  { $ev{owner} = $1 || '';}
  elsif (m|<tags>(.*)</tags>|)    { $ev{tags} = $1 || '';}
  elsif (m|</event>|){ $data{events} -> {$id} = {%ev}; }
  else {die "Error in event parsing!\n"; }
}
close IN;
print YAML::Tiny::Dump(\%data)
