#!/usr/bin/perl -w

# преобразование из старого формата в новый
# (yaml, отдельные записи для треков)
# более радикальное преобразование см.в ll9

use strict;
use YAML::Tiny;

my $datadir="./old";

opendir DATADIR, "$datadir"
  or die "can't open $datadir dir: $!\n";

while (my $f = readdir DATADIR){
  next if ($f =~ /^\./) || -d "$datadir/$f";

  unless (open IN, "$datadir/$f"){
    warn "skipping file $datadir/$f: $!\n";
    next;
  };

  print "processing $f\n";

  my $read=0;
  my %entry;
  my %data;
  my $id;

  foreach (<IN>){
    if ($read==0){
      if (/<!-- entry (\d+)\s+(\S+)\s+(\S+\s+\S+)\s+-->/){
        $read=1;
        %entry=();
        $id=$1;
        $entry{cuser} = $2;
        $entry{ctime} = $3;
        next;
      }
    }
    if ($read==1){
      if (/^title:\s+(.*)/){ $entry{title} = $1; next; }
      if (/^date:\s+(.*)/){  $entry{date} = $1; next; }
      if (/^tags:\s+(.*)/){  push @{$entry{tags}}, split /\s+/, $1; next; }
      if (/^> (.*)$/){
         if (!$entry{body}){ $entry{body} = $1;}
         else {$entry{body}.="\n".$1;}
         next;
      }

      if (/<!-- \/entry -->/){
        $data{$id} = {%entry};
        $read=0; next;
      }
    }
  }

  close IN;
  open OUT, "> $f.yml"
    or die("Can't open file $f.yml: $!");
  print OUT YAML::Tiny::Dump(\%data)
    or die("Can't write yaml: $!");
  close OUT;

}

