#!/usr/bin/perl -w
use strict;
use locale;

use CGI   ':standard';
use eventdb;

use POSIX qw(locale_h);
use locale;
setlocale(LC_ALL, "ru_RU.KOI8-R");


### get session, print html headers
my ($user, $pass, $level) = eventdb::login();

my %tags = {
1 => velo вело
2 => pesh пешком
3 => lyzh лыжи
4 => vodn сплав
5 => gorn горная техника
6 => sor  соревнования

101 => D группа Дмитриева
102 => S группа Сафронова

podm Подмосковье
evr  Европейская часть России
kavk Кавказ
ural Урал
sib  Сибирь
zagr Заграница

}

######################################################################
my %actions = (
login_form => sub($$$){
  my ($user, $pass, $level) = @_;
  print eventdb::form($user);
},
print_errors => sub($$$){
  return unless $eventdb::err;
  my ($user, $pass, $level) = @_;
  print qq* <h3 class=error>$eventdb::err</h3>*;
},

#### user list actions
user_list => sub($$$){
  my ($user, $pass, $level) = @_;
  my $out = eventdb::query($user, $pass, 'user_list');
  my @users=split(/\n/, $out);
  print qq*
  <h3>Список пользователей</h3>
  <ul>*;
  foreach (@users){
    my ($name, $act, $l) = split(/:/, $_);
    my $nt=$name;
    $nt="<s>$nt</s>" if $act==0;
    my $l1='';
    $l1 = ' -- admin' if ($l >= $eventdb::lvl_admin);
    print qq*
    <li><a href=\"eventdb.pl?user=$name\">$nt</a>$l1*;
  }
  print qq*
  </ul>*;
},
user_add_form =>sub($$$){
  my ($user, $pass, $level) = @_;
  print qq*
    <h3>Создать нового пользователя</h3>
    <form method="post" action="">
    <input name="user_list" type="hidden" value="1"/>
    имя: <input name="new_name" type="text" maxlength="15" size="10"/>
    пароль: <input name="new_pass" type="password" maxlength="15" size="10"/>
    <input type="submit" value="добавить" name="UserAdd"/>
    </form>* if $level >= $eventdb::lvl_admin;
},
user_add_op => sub($$$){
  my ($user, $pass, $level) = @_;
  my $new_name = param('new_name') || '';
  my $new_pass = param('new_pass') || '';
  eventdb::query($user, $pass, 'user_add', $new_name, $new_pass)
    if defined param('UserAdd');
},

#### user actions
user_print_name => sub($$$){
  print param('user') || '';
},
user_form => sub($$$){
  my ($user, $pass, $level) = @_;
  my $usr=param('user') || '';
  my $out = eventdb::query($user, $pass, 'user_show',  $usr);
  my ($name, $act, $l) = split(/:/, $out);
  my $l1='';
  $l1 = 'admin' if ($l >= $eventdb::lvl_admin);

  my $act_txt = $act? "да":"нет";
  my $btn_txt = $act? "деактивировать":"активировать";
  my $btn_val = $act? "UserOff":"UserOn";

  print qq*
    <h3>Информация о пользователе <font color=red>$usr</font>
    </h3>
    <form method="post" action="">
    <input name="user" type="hidden" value="$usr"/>*;
  if ($usr ne 'root'){
    print qq*
      <p>Активность: <b>$act_txt</b>*;
    print qq*
        <input type="submit" value="$btn_txt" name="$btn_val"/>*
          if $level >= $eventdb::lvl_admin;
    print qq*
      <p>Права: <b>$l1</b>*;
    print qq*
      <p>изменить права: <select name="new_level">
        <option name=normal value=$eventdb::lvl_norm> normal </option>
        <option name=admin  value=$eventdb::lvl_admin> admin </option></select>
        <input type="submit" value="ок" name="ChLevel"/>*
          if $level >= $eventdb::lvl_admin;
  }
  print qq*
    <p>Сменить пароль: <input name="new_pwd" type="password" maxlength="15" size="10"/>
      <input type="submit" value="сменить" name="UserChPwd"/>*
        if $level >= $eventdb::lvl_root || $user eq $usr ||
             ($level >= $eventdb::lvl_admin && $usr ne 'root' );
  print qq*
      </form>*;
},
user_change_op => sub($$$){
  my ($user, $pass, $level) = @_;

  my $usr=param('user') || '';
  eventdb::query($user, $pass, 'user_seta',  $usr, 'on')
    if defined param('UserOn');

  eventdb::query($user, $pass, 'user_seta', $usr, 'off')
    if defined param('UserOff');

  my $new_level = param('new_level') || '';
  eventdb::query($user, $pass, 'user_setl', $usr, $new_level)
    if defined param('ChLevel');

  if (defined param('UserChPwd')){
    my $new_pwd=param('new_pwd') || '';
    if ($user eq $usr){
      eventdb::query($user, $pass, 'user_mypwd', $new_pwd)
       and eventdb::chpwd($new_pwd) and $pass=$new_pwd;}
    else{
      eventdb::query($user, $pass, 'user_chpwd', $usr, $new_pwd);}
  }
},

#### search actions
search_controls => sub($$$){
  print qq*
    <form method="get">
    Искать текст: <input name="search_txt" type="text" maxlength="25" size="25"/>
    тэг: <input name="search_tag" type="text" maxlength="5" size="5"/>
    год: <input name="search_year" type="text" maxlength="5" size="5"/>
    <input type="submit" value="Искать" name="Search"/>
  *;
},

search_results => sub($$$){
  my $search_txt=param('search_txt') || '';
  my $search_tag=param('search_tag') || '';
  my $search_year=param('search_year') || '';
  my $out = eventdb::query($user, $pass, 'event_search',
    $search_txt, '', '', '', '', "-1", "-1", $search_tag);
  while ($out =~ s|<event id=(\d+) date1=(\d+) date2=(\d+)>(.*?)</event>||s){
    my ($id, $d1, $d2, $u) = ($1, $2, $3, $4);
    $d1=~s|(\d{4})(\d{2})(\d{2})|$1/$2/$3|; my ($dy1, $dm1, $dd1) = ($1,$2,$3);
    $d2=~s|(\d{4})(\d{2})(\d{2})|$1/$2/$3|; my ($dy2, $dm2, $dd2) = ($1,$2,$3);
    # packed date
    my $date = $d1;
    if    ($dy1 ne $dy2){ $date.="-$d2"; }
    elsif ($dm1 ne $dm2){ $date.="-$dm2/$dd2"; }
    elsif ($dd1 ne $dd2){ $date.="-$dd2"; }

    $u=~m|<title>(.*?)</title>|;   my $title = $1 || '';
    $u=~m|<people>(.*?)</people>|; my $people = "<b>Участники:</b> $1<br>\n" || '';
    $u=~m|<route>(.*?)</route>|;   my $route =  "<b>Маршрут:</b> $1<br>\n" || '';
    $u=~m|<body>(.*?)</body>|;     my $body = $1 || '';
    $u=~m|<tags>(.*?)</tags>|;     my $tags = $1 || '';

    print qq*
    <div id="eventh_$id" onclick="toggle('$id');" style="padding: 5;">
    <b style="cursor: pointer;"><i>$date:</i> $title</b>
    <div id="event_$id" style="display:none; padding: 5;"><hr>
    $people$route$body
    $tags
      <div align=right> <a href="eventdb.pl?event=$id">[ссылка]</a> </div>
    </div>
    </div>
    *;
  }
},
last_large_events => sub($$$){
  print qq*
    <h3>Большие походы</h3>*;
},
last_changes => sub($$$){
  print qq*
    <h3>Последние изменения</h3>*;
},

event_form => sub($$$){
  my $event  = param('event') || '';
  my ($title, $d1, $d2, $people, $route, $body, $tags)  = ('','','','','','','','');
  if ($event){
    my $out = eventdb::query($user, $pass, 'event_show', $event);
    while ($out =~ s|<event id=(\d+) date1=(\d+) date2=(\d+)>(.*?)</event>||s){
      ($d1, $d2) = ($2, $3);
      my $u = $4;
      $d1=~s|(\d{4})(\d{2})(\d{2})|$1-$2-$3|;
      $d2=~s|(\d{4})(\d{2})(\d{2})|$1-$2-$3|;
      $u=~m|<title>(.*?)</title>|;   $title  = $1 || '';
      $u=~m|<people>(.*?)</people>|; $people = $1 || '';
      $u=~m|<route>(.*?)</route>|;   $route  = $1 || '';
      $u=~m|<body>(.*?)</body>|;     $body = $1 || '';
      $u=~m|<tags>(.*?)</tags>|;     $tags = $1 || '';
    }
  }
  print qq*
  <form method="post" enctype="multipart/form-data">
    <input name="event" type="hidden" value="$event">
    <p>Заголовок:  <input name="title" type="text" maxlength="245" size="60" value="$title">
    <p>Даты (yyyy-mm-dd):
      <input name="date1" type="text" maxlength="12" size="12" value="$d1"> -
      <input name="date2" type="text" maxlength="12" size="12" value="$d2"><br>
    <p>Участники:<br>
      <textarea name="people" rows=3 cols=85 wrap=soft>$people</textarea>
    <p>Маршрут:<br>
      <textarea name="route" rows=5 cols=85 wrap=soft>$route</textarea>
    <p>Текст:<br>
      <textarea name="body" rows=10 cols=85 wrap=soft>$body</textarea>
    <p>
    <p>Метки:<br>

    <p><input type="submit" name="EventEdit" value="Сохранить запись">
    <input type="submit" name="EventDel" value="Удалить запить">
    </form>
  </form>*;
},

event_change_op => sub($$$){
  my ($user, $pass, $level) = @_;

  my $event=param('event') || '';

  if (defined param('EventEdit')){
    my $title  = param('title')  || '';
    my $body   = param('body')   || '';
    my $people = param('people') || '';
    my $route  = param('route')  || '';
    my $date1  = param('date1')  || '';
    my $date2  = param('date2')  || $date1;
    my $tags   = param('tags')  || '';
    $date1=~tr|.:/\\-||d;
    $date2=~tr|.:/\\-||d;
    if ($event){
      eventdb::query($user, $pass, 'event_edit',  $event,
        $title, $body, $people, $route, $date1, $date2, $tags);}
    else {
      my $ev = eventdb::query($user, $pass, 'event_create',
        $title, $body, $people, $route, $date1, $date2, $tags);
      param('event', $ev);
    }
  }

  eventdb::query($user, $pass, 'event_delete',  $event)
    if $event && defined param('EventDel');
},


);

######################################################################

sub process_template($$$$$){
  my ($file, $user, $pass, $level, $actions) = @_;
  unless (open IN, $file) {
    print qq*
      <html><body><h3>No template: $file</h3></body></html>*;
  }
  foreach(<IN>){
    if (/^\\(\S+)/){
      my $cmd = $1;
      if (exists $actions->{$cmd}){
        print "<!-- $cmd -->";
        $actions->{$cmd}($user, $pass, $level);
        print "\n<!-- /$cmd -->\n";
      }
      else {print "<!-- unknown command: $cmd -->\n";}
    }
    else {print;}
  }
}

######################################################################

if (defined param('user')){
  process_template("evdb_user.htm", $user, $pass, $level, \%actions); }
elsif (defined param('user_list')){
  process_template("evdb_user_list.htm", $user, $pass, $level, \%actions); }
elsif (defined param('event') && !defined param('EventDel')){
  process_template("evdb_event.htm", $user, $pass, $level, \%actions); }
elsif (defined param('geo') && !defined param('GeoDel')){
  process_template("evdb_geo.htm", $user, $pass, $level, \%actions); }
else {
  process_template("evdb_main.htm", $user, $pass, $level, \%actions); }
