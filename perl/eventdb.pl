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
print qq*<html><body>
  <table border=1 cellpadding=10 cellspacing=0>
  <tr><td align=right bgcolor="#EECCEE">*;
print eventdb::form($user);
print qq*</td></tr>*;



# show events

  my $out = eventdb::query($user, $pass, 'envent_search', "", "", "", "", "", "", "");



  my ($name, $act, $l) = split(/:/, $out);
  my $l1='';
  $l1 = 'admin' if ($l >= $eventdb::lvl_admin);

  my $act_txt = $act? "да":"нет";
  my $btn_txt = $act? "деактивировать":"активировать";
  my $btn_val = $act? "UserOff":"UserOn";

  print qq*
    <tr><td bgcolor="#FFEEFF">
    <h3>Информация о пользователе <font color=red>$name</font></h3><form>
    <form method="post" action="">
    <input name="usr" type="hidden" value="$usr"/>*;
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
        <option name=normal value=0> normal </option>
        <option name=admin  value=99> admin </option></select>
        <input type="submit" value="ок" name="ChLevel"/>*
          if $level >= $eventdb::lvl_admin;
  }
  print qq*
    <p>Сменить пароль: <input name="new_pwd" type="password" maxlength="15" size="10"/>
      <input type="submit" value="сменить" name="UserChPwd"/>*
         if $level >= $eventdb::lvl_admin || $user eq $usr;
  print qq*
    </form>
    <hr width="70%">
    <p><a href="eventdb_users.pl">Вернуться к списку пользователей...</a>*;
}

else{ ## show user list
  my $out = eventdb::query($user, $pass, 'user_list');
  my @users=split(/\n/, $out);
  print qq*
    <tr><td bgcolor="#FFEEFF">
    <h3>Список пользователей:</h3>
    <ul>*;
  foreach (@users){
    my ($name, $act, $l) = split(/:/, $_);
    my $nt=$name;
    $nt="<s>$nt</s>" if $act==0;
    my $l1='';
    $l1 = ' -- admin' if ($l >= $eventdb::lvl_admin);
    print "     <li><a href=\"eventdb_users.pl?usr=$name\">$nt</a>$l1\n";
  }
  print qq*
    </ul>*;
  print qq*
    <hr width="70%">
    <p>Добавить пользователя:
    <form method="post" action="">
    имя: <input name="new_name" type="text" maxlength="15" size="10"/>
    пароль: <input name="new_pass" type="password" maxlength="15" size="10"/>
    <input type="submit" value="добавить" name="UserAdd"/>
    </form>* if $level >= $eventdb::lvl_admin;
}

## print html tail
print qq*
  </td></tr>
  </table>
  </body></html>\n*;
