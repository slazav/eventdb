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
  eventdb::query($user, $pass, 'user_on',  $usr)
    if defined param('UserOn');

  eventdb::query($user, $pass, 'user_off', $usr)
    if defined param('UserOff');

  my $new_level = param('new_level') || '';
  eventdb::query($user, $pass, 'user_chlvl', $usr, $new_level)
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

user_add_op => sub($$$){
  my ($user, $pass, $level) = @_;
  my $new_name = param('new_name') || '';
  my $new_pass = param('new_pass') || '';
  eventdb::query($user, $pass, 'user_add', $new_name, $new_pass)
    if defined param('UserAdd');
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
else {
  process_template("evdb_main.htm", $user, $pass, $level, \%actions); }
