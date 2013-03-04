#!/usr/bin/perl -w

# �������������� �� ������� ������� � �����
# (yaml, ��������� ������ ��� ������)

use strict;
use YAML::Tiny;

my $indir="./old";

my %data;

my %tag_nums=(
  D => 11, # ������ ���������
  S => 12, # ������ ���������

  podm => 21, # �����������
  evr  => 22, # ����������� ����� ������
  kavk => 23, # ������
  ural => 24, # ����
  sib  => 25, # ������
  zagr => 26, # ���������

  velo => 31, # ����
  pesh => 32, # ������
  lyzh => 33, # ����
  vodn => 34, # �����

  sor  => 41, # ������������
  '1day' => 42, # ��� (1-2���)
  mday => 43, # ������������ ������
);

sub decode_date{
  my $in=shift;
  my $y = '(\d{4})';
  my $m = '(\d{1,2})';
  my $ymm = "$y/$m/$m";
  if ($in =~ m|^$y\s*$|) {return "${1}0000", "${1}0000";}
  if ($in =~ m|^$y/$m\s*$|) {return "${1}${2}00", "${1}${2}00";}
  if ($in =~ m|^$ymm\s*$|) {return "${1}${2}${3}", "${1}${2}${3}";}
  if ($in =~ m|^$ymm\s*-\s*$m\s*$|) {return "${1}${2}${3}", "${1}${2}${4}";}
  if ($in =~ m|^$ymm\s*-\s*$m/$m\s*$|) {return "${1}${2}${3}", "${1}${4}${5}";}
  if ($in =~ m|^$ymm\s*-\s*$ymm\s*$|) {return "${1}${2}${3}", "${4}${5}${6}";}
  if ($in =~ m|^$y/$m\s*-\s*$y/$m\s*$|) {return "${1}${2}00", "${3}${4}00";}
  print STDERR "WARNING: wrong date: <<$in>>\n";
  return 0, 0;
}

sub decode_links{
  my @links;
  while ($_[0]=~s/���������:\s*\(\((\S+)\s+(.*?)\)\)//s){
    push @links, {url=>$1, text=>"���������: $2", tags=>2};
  }
  while ($_[0]=~s/\(\((\S+)\s+(.*?)\)\)//s){
    my $tags='';
    $tags=1 if ($2 && $2=~/����/i);
    $tags=2 if ($2 && $2=~/���/i);
    push @links, {url=>$1, text=>$2, tags=>$tags};
  }
  return [@links];
}

sub cleanup_body{
  $_[0]=~s/(\s)\s+/$1/sg;
  $_[0]=~s/\s+$//;
}

opendir DATADIR, "$indir"
  or die "can't open $indir dir: $!\n";

while (my $f = readdir DATADIR){
  next if ($f =~ /^\./) || -d "$indir/$f";

  unless (open IN, "$indir/$f"){
    warn "skipping file $indir/$f: $!\n";
    next;
  };

  print STDERR "processing $f\n";

  my $read=0;
  my %entry;
  my $id;

  foreach (<IN>){
    if ($read==0){
      if (/<!-- entry (\d+)\s+(\S+)\s+(\S+\s+\S+)\s+-->/){
        $read=1;
        %entry=();
        $id=$1;
        $entry{owner} = $2;
        $entry{ctime} = `date -d "$3" +%s`;
        $entry{mtime} = time;
        chomp $entry{ctime};
        next;
      }
    }
    if ($read==1){
      if (/^title:\s+(.*)/){ $entry{title} = $1; next; }
      if (/^date:\s+(.*)/){  ($entry{date1},$entry{date2}) = decode_date($1); next; }
      if (/^tags:\s+(.*)/){
         my @tags = split /\s+/, $1;
         $_ = $tag_nums{$_} || '' foreach (@tags);
         $entry{tags} = join ',', @tags;
         next;
      }
      if (/^> (.*)$/){
         if (!$entry{body}){ $entry{body} = $1;}
         else {$entry{body}.="\n".$1;}
         next;
      }

      if (/<!-- \/entry -->/){
        if ($entry{body}){
          $entry{links} = decode_links $entry{body};
          cleanup_body $entry{body};
        }
        $data{$id} = {%entry};
        $read=0; next;
      }
    }
  }

  print YAML::Tiny::Dump(\%data)

}

