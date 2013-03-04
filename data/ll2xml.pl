#!/usr/bin/perl -w

# преобразование из старого формата в новый

use strict;

my $indir="./old";

my %data;

my %tag_nums=(
  D => 11, # группа Дмитриева
  S => 12, # группа Сафронова

  podm => 21, # Подмосковье
  evr  => 22, # Европейская часть России
  kavk => 23, # Кавказ
  ural => 24, # Урал
  sib  => 25, # Сибирь
  zagr => 26, # Заграница

  velo => 31, # вело
  pesh => 32, # пешком
  lyzh => 33, # лыжи
  vodn => 34, # сплав

  sor  => 41, # соревнования
  '1day' => 42, # ПВД (1-2дня)
  mday => 43, # многодневные походы
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
  return 'В.Завьялов'     if match_name($_[0], 'Владиславa?', 'Слав(а|ы)', 'Завьялова?');
  return 'А.Тонис'        if match_name($_[0], 'Александра?', 'Саш(а|и)',  'Тониса?');
  return 'А.Веретенников' if match_name($_[0], 'Александра?', 'Саш(а|и)',  'Веретенникова?');
  return 'А.Чупикин'      if match_name($_[0], 'Андре(й|я)', '', 'Чупикина?');
  return 'Д.Шварц'        if match_name($_[0], 'Дмитри(й|я)', 'Дим(а|ы)', 'Шварца?');
  return 'Д.Савватеев'    if match_name($_[0], 'Дмитри(й|я)', 'Дим(а|ы)', 'Савватеева?');
  return 'А.Савватеев'    if match_name($_[0], 'Алексе(й|я)', 'Л(е|ё)ш(а|ы)', 'Савватеева?');
  return 'А.Петров'       if match_name($_[0], 'Алексе(й|я)', 'Л(е|ё)ш(а|ы)', 'Петрова?');
  return 'А.Ерошин'       if match_name($_[0], 'Алексе(й|я)', 'Л(е|ё)ш(а|ы)', 'Ерошина?');
  return 'А.Бахвалов'     if match_name($_[0], 'Александра?', 'Саш(а|и)', 'Бахвалова?');
  return 'О.Чхетиани'     if match_name($_[0], 'Отто', 'Антона?', 'Чхетиани');
  return 'Н.Осадчий'      if match_name($_[0], 'Никола(й|я)', 'Кол(я|и)', 'Осадч(ий|его)');
  return 'М.Осадчий'      if match_name($_[0], 'Михаила?',    'Миш(а|и)', 'Осадч(ий|его)');
  return 'Л.Фишкис'       if match_name($_[0], 'Леонида?',    'Л(е|ён)(я|и)', 'Фишкиса?');
  return 'М.Агеев'        if match_name($_[0], 'Михаила?',    'Миш(а|и)', 'Агеева?');
  return 'Н.Смыслов'      if match_name($_[0], 'Никола(й|я)', 'Кол(я|и)', 'Смыслова?');
  return 'С.Булычев'      if match_name($_[0], 'Серге(й|я)',  '', 'Булычева?');
  return 'Т.Алексеевский' if match_name($_[0], 'Тимофе(й|я)',  'Тима?', 'Алексеевск(ий|ого)');
  return 'Д.Будяк'        if match_name($_[0], 'Дениса?', '', 'Будяка?');
  return 'Д.Зуев'         if match_name($_[0], 'Дениса?', '', 'Зуева?');
  return 'О.Волков'       if match_name($_[0], 'Олега?', '',  'Волкова?');
  return 'А.Сорокин'      if match_name($_[0], 'Антона?', '',  'Сорокина?');
  return 'К.Шрамов'       if match_name($_[0], 'Константина?', 'Кост(я|и)',  'Шрамова?');
  return 'К.Павлов'       if match_name($_[0], 'Кирилла?', '', 'Павлова?');
  return 'В.Дубицкий'     if match_name($_[0], 'Виктора?', 'Вит(я|и)', 'Дубицк(ий|ого)');
  return 'В.Осетров'      if match_name($_[0], 'Виктора?', 'Вит(я|и)', 'Осетрова?');
  return 'Д.Терентьева'   if match_name($_[0], 'Дин(а|ы)', '', 'Терентьев(а|ой)');
  return 'А.Акимова'      if match_name($_[0], 'Ален(а|ы)', '', 'Акимов(а|ой)');
  return 'Н.Банникова'    if match_name($_[0], 'Нин(а|ы)', '', 'Банников(а|ой)');
}

sub get_ltags{
  my @tags=();
  if ($_[0]=~s/^\s*фото(отчет|альбом|графии)?\s*//i){ push @tags, 1;}
  if ($_[0]=~s/^\s*трек:?\s*//i ||
      $_[0]=~s/^\s*геоданные:?\s*//i) {push @tags, 2;}
  if ($_[0]=~s/^\s*приглашение\s*//i) {push @tags, 10;}
  if ($_[0]=~s/^\s*впечатления\s*//i) {push @tags, 11;}
  return join ',', @tags;
}

sub get_gtags{
  my $url=shift;
  my $text=shift;
  my @tags=();
  if (($url && $url =~ /pr\.zip$/) ||
      ($text && $text =~ /проект/)) { push @tags, 10;}
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
          if (s/^геоданные:\s*\(\((\S+)\s+(.*?)\)\)$//){
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

