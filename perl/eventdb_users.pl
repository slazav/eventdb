#!/usr/bin/perl -w
use strict;
use locale;

use CGI   ':standard';
use eventdb;

use POSIX qw(locale_h);
use locale;
setlocale(LC_ALL, "ru_RU.KOI8-R");

### print error in a table raw
sub print_err{
  my $err = shift;
  $err=~s/</&lt;/g;
  $err=~s/>/&gt;/g;
  $err=~s/\n/<br>/g;
  print qq*
    <tr><td bgcolor="#FFDDDD"><h3 color=red>$err</h3></td></tr>*;
}

### is user in the user_edit group?
sub check_user_edit{
  my $usr = shift;
  return 1 if $usr eq 'root';
  return 0 if $usr eq '';
  my $out=query('','','user_show', $usr);
  return $out=~/$usr:\d.*(\s|:)user_edit(\s|$)/;
}

### query to eventdb (result is returned, error is printed)
sub query{
  my $user   = shift;
  my $pass   = shift;
  my $action = shift;
  my $args = '';
  foreach (@_){
    s/'/'"'"'/g; #'# A single quote may not occur between single quotes,
                   # even when preceded by a backslash -- man bash
    $args .= " '$_'";
  }
  my $out = qx($eventdb::evdb_prog '$user' '$pass' '$action' $args 2>&1) || '';
  my $ret = $? >> 8;

  print_err($out) if $ret != 0;
  return $ret==0? $out:'';
}


### get session, print html headers
my ($user, $pass) = eventdb::login();
print qq*<html><body>
  <table border=1 cellpadding=10 cellspacing=0>
  <tr><td align=right bgcolor="#EECCEE">*;
print eventdb::form($user);
print qq*
  </tr></td>*;

### do some actions (and print errors if needed)
my $usr=param('usr') || '';
query($user, $pass, 'user_on',  $usr) if defined param('UserOn');
query($user, $pass, 'user_off', $usr) if defined param('UserOff');
query($user, $pass, 'user_add',
  param('new_name') || '', param('new_pass') || '' ) if defined param('UserAdd');
query($user, $pass, 'group_add',
  $usr, param('group') || '') if defined param('GroupAdd');
query($user, $pass, 'group_del',
  $usr, param('group') || '') if defined param('GroupDel');
if (defined param('UserChPwd')){
  my $new_pwd=param('new_pwd') || '';
  if ($new_pwd=~/^.{0,4}$/){
    print_err("Error: password is too short!");
  }
  else{
    query($user, $pass, 'user_chpwd', $usr, $new_pwd)
      and eventdb::chpwd($new_pwd) and $pass=$new_pwd;
  }
}

my $can_edit = check_user_edit($user);

if ($usr){ ## show one user
  my $out = query($user, $pass, 'user_show',  $usr);
  my ($name, $act, $groups) = split(/:/, $out);
  $groups=~s/^\s+//g; $groups=~s/\s+$//g;
  my $act_txt = $act? "да":"нет";
  my $btn_txt = $act? "деактивировать":"активировать";
  my $btn_val = $act? "UserOff":"UserOn";
  $groups = '-- нет --' if !$groups;

  print qq*
    <tr><td bgcolor="#FFEEFF">
    <h3>Информация о пользователе <font color=red>$name</font></h3><form>
    <form method="post" action="">
    <input name="usr" type="hidden" value="$usr"/>*;
  if ($usr ne 'root'){
    print qq*
      <p>Активность: <b>$act_txt</b>*;
    print qq*
        <input type="submit" value="$btn_txt" name="$btn_val"/>* if $can_edit;
    print qq*
      <p>Группы: <b>$groups</b>*;
    print qq*
      <p>группа: <input name="group" type="text" maxlength="15" size="10"/>
        <input type="submit" value="добавить" name="GroupAdd"/>
        <input type="submit" value="удалить" name="GroupDel"/>* if $can_edit;
  }
  print qq*
    <p>Сменить пароль: <input name="new_pwd" type="password" maxlength="15" size="10"/>
      <input type="submit" value="сменить" name="UserChPwd"/>* if $user eq 'root' || $user eq $usr;
  print qq*
    </form>
    <hr width="70%">
    <p><a href="eventdb_users.pl">Вернуться к списку пользователей...</a>*;
}

else{ ## show user list
  my $out = query($user, $pass, 'user_list');
  my @users=split(/\n/, $out);
  print qq*
    <tr><td bgcolor="#FFEEFF">
    <h3>Список пользователей:</h3>
    <ul>*;
  foreach (@users){
    my ($name, $act, $groups) = split(/:/, $_);
    my $nt=$name;
    $nt="<s>$nt</s>" if $act==0;
    $groups=~s/^\s+//g; $groups=~s/\s+$//g;
    $groups=" (группы: <b>$groups</b>)" if $groups;
    print "     <li><a href=\"eventdb_users.pl?usr=$name\">$nt</a> $groups\n";
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
    </form>* if $can_edit;
}

## print html tail
print qq*
  </td></tr>
  </table>
  </body></html>\n*;
