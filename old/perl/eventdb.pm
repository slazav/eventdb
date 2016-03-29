package eventdb;

use strict;
use warnings;
use CGI   ':standard';
use CGI::Session;

our $evdb_prog="/usr/home/slazav/eventdb/eventdb";
our $lvl_root = 100;
our $lvl_admin = 99;
our $lvl_norm  =  1;
our $err = '';

my $session;

# get name/pass/LogIn/LogOut parameters, create/find/delete session,
# check password using eventdb program, print html headers.
sub login{
  $session = new CGI::Session();
  $session->expire('+48h');
  print $session->header('charset' => 'koi8-r');

  if (defined param('LogIn')) {
    my $name=param('name') || '';
    my $pass=param('pass') || '';
    return '','',0 if $name eq '';

    my $level = qx($evdb_prog "$name" "$pass" "user_check");
    my $ret = $? >> 8;

    if ($ret == 0){
      $session->param('name', $name);
      $session->param('pass', $pass);
      $session->param('level', $level);
      $session->flush();
      return $name, $pass, $level;
    } else {
      $session->delete();
      return '','',0;
    }
  }
  elsif (defined param('LogOut')) {
    $session->delete();
    return '','',0;
  }
  else{
    my $name=$session->param('name') || '';
    my $pass=$session->param('pass') || '';
    my $level=$session->param('level') || 0;
    return $name, $pass, $level;
  }
}

# login form
sub form{
  my $name=shift;
  my $url=shift;
  if ($name eq ''){
    return qq*
    <form method="post" action="">
    имя: <input name="name" type="text" maxlength="15" size="10"/>
    пароль: <input name="pass" type="password" maxlength="15" size="10"/>
    <input type="submit" value="войти" name="LogIn"/></form>*;
  } else {
    return qq*
    <form method="post" action="">
    <b>$name</b>
    <input type="submit" value="выйти" name="LogOut"/></form>*;
  }
}

## NOT WORKING!!!
sub chpwd{
  my $pass=shift;
  $session->param('pass', $pass);
  $session->flush();
}

### print error in a table raw
sub print_err{
  my $err = shift;
  $err=~s/</&lt;/g;
  $err=~s/>/&gt;/g;
  $err=~s/\n/<br>/g;
  print qq*
    <tr><td bgcolor="#FFDDDD"><h3 color=red>$err</h3></td></tr>*;
}

### query to eventdb (result is returned, $err is set)
sub query{
  my $user   = shift;
  my $pass   = shift;
  my $action = shift;
  my $args = '';
  foreach (@_){
    my $l = $_;
    $l =~ s/'/'"'"'/g; #'# A single quote may not occur between single quotes,
                   # even when preceded by a backslash -- man bash
    $args .= " '$l'";
  }
  my $out = qx($eventdb::evdb_prog '$user' '$pass' '$action' $args 2>&1) || '';
  my $ret = $? >> 8;

  $err = $err . "<br>" . $out if $ret != 0;
  return $ret==0? $out:'';
}

1;
