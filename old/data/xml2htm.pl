#!/usr/bin/perl -w

use strict;

sub date_pack($$){
  my $d1=shift;
  my $d2=shift;
  $d1=~s|(\d{4})(\d{2})(\d{2})|$1/$2/$3|; my ($dy1, $dm1, $dd1) = ($1,$2,$3);
  $d2=~s|(\d{4})(\d{2})(\d{2})|$1/$2/$3|; my ($dy2, $dm2, $dd2) = ($1,$2,$3);
  if    ($dy1 ne $dy2){ return "$d1-$d2"; }
  elsif ($dm1 ne $dm2){ return "$d1-$dm2/$dd2"; }
  elsif ($dd1 ne $dd2){ return "$d1-$dd2"; }
  return $d1;
}

sub print_html($){
  my $ev=shift;
  my $date=date_pack($ev->{date1}, $ev->{date2});

  print qq*
    <div id="eventh_$ev->{id}" onclick="toggle('$ev-{id}');" style="padding: 5;">
    <b style="cursor: pointer;"><i>$date:</i> $ev->title</b>
    <div id="event_$id" style="display:none; padding: 5;"><hr>*;

}


open IN ,$ARGV[0]) or die "Can't read $ARGV[0]: $!\n";
my $in=<IN>;
close IN;

while ($in=~s/<event\s+id=(\d+)\s+>(.*?)</event>//ims){
  my %ev;
  $ev{id}=$1;
  $ev{raw}=$2;

  $ev{raw}=~m|<ctime>(.*?)</ctime>|;   my $ev{ctime}  = $1 || '';
  $ev{raw}=~m|<mtime>(.*?)</mtime>|;   my $ev{mtime}  = $1 || '';
  $ev{raw}=~m|<owner>(.*?)</owner>|;   my $ev{owner}  = $1 || '';
  $ev{raw}=~m|<title>(.*?)</title>|;   my $ev{title}  = $1 || '';
  $ev{raw}=~m|<people>(.*?)</people>|; my $ev{people} = $1 || '';
  $ev{raw}=~m|<route>(.*?)</route>|;   my $ev{route}  = $1 || '';
  $ev{raw}=~m|<body>(.*?)</body>|;     my $ev{body}   = $1 || '';
  $ev{raw}=~m|<tags>(.*?)</tags>|;     my $ev{tags}   = $1 || '';

  while ($ev{raw}=~s/<link\s+id=(\d+)\s+>(.*?)</link>//ims){
    my %ln;
    $ln{id}=$1;
    $ln{raw}=$2;
    $ln{raw}=~m|<ctime>(.*?)</ctime>|;   my $ln{ctime} = $1 || '';
    $ln{raw}=~m|<mtime>(.*?)</mtime>|;   my $ln{mtime} = $1 || '';
    $ln{raw}=~m|<owner>(.*?)</owner>|;   my $ln{owner} = $1 || '';
    $ln{raw}=~m|<url>(.*?)</url>|;       my $ln{url}   = $1 || '';
    $ln{raw}=~m|<text>(.*?)</text>|;     my $ln{text}  = $1 || '';
    $ln{raw}=~m|<tags>(.*?)</tags>|;     my $ln{tags} = $1 || '';
    push @{$ev{links}}, %ln;
  }

  while ($ev{raw}=~s/<geo\s+id=(\d+)\s+>(.*?)</geo>//ims){
    my %gg;
    $gg{id}=$1;
    $gg{raw}=$2;
    $gg{raw}=~m|<ctime>(.*?)</ctime>|;   my $gg{ctime} = $1 || '';
    $gg{raw}=~m|<mtime>(.*?)</mtime>|;   my $gg{mtime} = $1 || '';
    $gg{raw}=~m|<owner>(.*?)</owner>|;   my $gg{owner} = $1 || '';
    $gg{raw}=~m|<url>(.*?)</url>|;       my $gg{url}   = $1 || '';
    $gg{raw}=~m|<text>(.*?)</text>|;     my $gg{text}  = $1 || '';
    $gg{raw}=~m|<tags>(.*?)</tags>|;     my $gg{tags} = $1 || '';
    push @{$ev{links}}, %gg;
  }

  print_html(\%ev);
}

