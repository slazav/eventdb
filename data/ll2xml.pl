#!/usr/bin/perl -w

# �������������� �� ������� ������� � �����

use strict;

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
  if ($in =~ m|^$y/$m/$m|) {return "${1}${2}${3}", "${1}${2}}{3}";}
  print STDERR "WARNING: wrong date: <<$in>>\n";
  return 0, 0;
}

sub match_name{
  my $n1=$_[1];
  my $n2=$_[2];
  my $fa=$_[3];
  $n1=~/^(.)/; my $n3=$1.'\.';
  my $nn=join '|', ($n1, $n2, $n3);
  return ($_[0]=~s/,?\s*($nn)\s*$fa//);
}

sub get_auth{
  return '' unless $_[0];
  return '�.��������'     if match_name($_[0], '���������a?', '����(�|�)', '���������?');
  return '�.�����'        if match_name($_[0], '����������?', '���(�|�)',  '������?');
  return '�.������������' if match_name($_[0], '����������?', '���(�|�)',  '�������������?');
  return '�.�������'      if match_name($_[0], '�����(�|�)', '', '��������?');
  return '�.�����'        if match_name($_[0], '������(�|�)', '���(�|�)', '������?');
  return '�.���������'    if match_name($_[0], '������(�|�)', '���(�|�)', '����������?');
  return '�.���������'    if match_name($_[0], '������(�|�)', '�(�|�)�(�|�)', '����������?');
  return '�.������'       if match_name($_[0], '������(�|�)', '�(�|�)�(�|�)', '�������?');
  return '�.������'       if match_name($_[0], '������(�|�)', '�(�|�)�(�|�)', '�������?');
  return '�.��������'     if match_name($_[0], '����������?', '���(�|�)', '���������?');
  return '�.��������'     if match_name($_[0], '����', '������?', '��������');
  return '�.�������'      if match_name($_[0], '������(�|�)', '���(�|�)', '�����(��|���)');
  return '�.�������'      if match_name($_[0], '�������?',    '���(�|�)', '�����(��|���)');
  return '�.������'       if match_name($_[0], '�������?',    '�(�|��)(�|�)', '�������?');
  return '�.�����'        if match_name($_[0], '�������?',    '���(�|�)', '������?');
  return '�.�������'      if match_name($_[0], '������(�|�)', '���(�|�)', '��������?');
  return '�.�������'      if match_name($_[0], '�����(�|�)',  '', '��������?');
  return '�.������������' if match_name($_[0], '������(�|�)',  '����?', '����������(��|���)');
  return '�.�����'        if match_name($_[0], '������?', '', '������?');
  return '�.����'         if match_name($_[0], '������?', '', '�����?');
  return '�.������'       if match_name($_[0], '�����?', '',  '�������?');
  return '�.�������'      if match_name($_[0], '������?', '',  '��������?');
  return '�.������'       if match_name($_[0], '�����������?', '����(�|�)',  '�������?');
  return '�.������'       if match_name($_[0], '�������?', '', '�������?');
  return '�.��������'     if match_name($_[0], '�������?', '���(�|�)', '������(��|���)');
  return '�.�������'      if match_name($_[0], '�������?', '���(�|�)', '��������?');
  return '�.����������'   if match_name($_[0], '���(�|�)', '', '���������(�|��)');
  return '�.�������'      if match_name($_[0], '����(�|�)', '', '������(�|��)');
  return '�.���������'    if match_name($_[0], '���(�|�)', '', '��������(�|��)');
}

sub get_ltags{
  my @tags=();
  if ($_[0]=~s/^\s*����(�����|������|������)?\s*//i){ push @tags, 1;}
  if ($_[0]=~s/^\s*����:?\s*//i ||
      $_[0]=~s/^\s*���������:?\s*//i) {push @tags, 2;}
  if ($_[0]=~s/^\s*�����������\s*//i) {push @tags, 10;}
  if ($_[0]=~s/^\s*�����������\s*//i) {push @tags, 11;}
  return join ',', @tags;
}

sub get_gtags{
  my $url=shift;
  my $text=shift;
  my @tags=();
  if (($url && $url =~ /pr\.zip$/) ||
      ($text && $text =~ /������/)) { push @tags, 10;}
  return join ',', @tags;
}

sub print_link{
  my $url=shift;
  my $text=shift;
  my $ln;
  my $tags;

  if ($url=~/slazav.mccme.ru\/gps/){
    $ln = 'geo';
    $tags=get_gtags($url, $text);
  }
  else{
    $ln = 'link';
    $tags=get_ltags($text);
  }

  my $auth=get_auth($text);

  print " <$ln id=0>\n";
  print "   <url>$url</url>\n";
  print "   <text>$text</text>\n";
  print "   <auth>$auth</auth>\n";
  print "   <tags>$tags</tags>\n";
  print " </$ln>\n";
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
  my @body;

  foreach (<IN>){
    if ($read==0){
      if (/<!-- entry (\d+)\s+(\S+)\s+(\S+\s+\S+)\s+-->/){
        $read=1;
        @body=();
        my ($id, $owner) = ($1, $2);
        my $ctime = `date -d "$3" +%s`;
        chomp $ctime;
        my $mtime = time;
        print "<entry id=0>\n";
        print " <ctime>$ctime</ctime>\n";
        print " <mtime>$mtime</mtime>\n";
        print " <owner>$owner</owner>\n";
        next;
      }
    }
    if ($read==1){
      if (/^title:\s+(.*)/){ print "<title>$1</title>\n"; next; }
      if (/^date:\s+(.*)/){
        my ($date1, $date2) = decode_date($1);
        print " <date1>$date1</date1>\n";
        print " <date2>$date2</date2>\n";
        next;
      }
      if (/^tags:\s+(.*)/){
        my @tags = split /\s+/, $1;
        $_ = $tag_nums{$_} || '' foreach (@tags);
        my $tags = join ',', @tags;
        print " <tags>$tags</tags>\n";
        next;
      }
      if (/^> (.*)$/){
         push @body, $1;
         next;
      }

      if (/<!-- \/entry -->/){
        # decode links
        foreach (@body){
          if (s/^\(\((\S+)\s+(.*?)\)\)$//){
            print_link($1, $2);
          }
          if (s/^���������:\s*\(\((\S+)\s+(.*?)\)\)$//){
            print_link($1, $2);
          }
        }
        my $body=join "\n", @body;
        $body=~s/(\s)\s+/$1/sg;

        print " <body>$body</body>\n";
        print "</entry>\n";
        $read=0; next;
      }
    }
  }

}

