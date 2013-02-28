#!/usr/bin/perl -w

# �������������� �� ������� ������� � �����
# (yaml, ��������� ������ ��� ������)
# ����� ����������� �������������� ��.� ll9

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
  if ($in =~ m|^(\d{4})$|) {return "${1}0000", "${1}0000";}
  if ($in =~ m|^(\d{4})/(\d{2})$|) {return "${1}${2}00", "${1}${2}00";}
  if ($in =~ m|^(\d{4})/(\d{2})/(\d{2})$|) {return "${1}${2}${3}", "${1}${2}${3}";}
  if ($in =~ m|^(\d{4})/(\d{2})/(\d{2})-(\d{2})$|) {return "${1}${2}${3}", "${1}${2}${4}";}
  if ($in =~ m|^(\d{4})/(\d{2})/(\d{2})-(\d{2})/(\d{2})$|) {return "${1}${2}${3}", "${1}${4}${5}";}
  if ($in =~ m|^(\d{4})/(\d{2})/(\d{2})-(\d{4})/(\d{2})/(\d{2})$|) {return "${1}${2}${3}", "${4}${5}${6}";}
  print STDOUT "WARNING: wrong date: $in";
  return 0, 0;
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
        $data{$id} = {%entry};
        $read=0; next;
      }
    }
  }

  print YAML::Tiny::Dump(\%data)

}

