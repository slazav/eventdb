#!/usr/bin/perl

use strict;
use warnings;

# cleanup data folders
`rm -fr   -- ./data ./logs ./files`;
`mkdir -p -- ./data ./logs ./files`;

sub run_test{
  my $args = shift;
  my $sec  = shift;
  my $exp  = shift;
  my $res  = `printf "%s" "$sec" | ./eventdb $args`;
  my $out;
  $res =~ s/\"stime\": (\d+)/\"stime\": 1234567890/;
  $out->{stime}=$1;
  $res =~ s/\"session\": \"([^\"]+)\"/\"session\": \"-\"/;
  $out->{session}=$1;
  chomp($res);
  if ($res ne $exp){
    print "ERROR:\n";
    print "  res:$res\n";
    print "  exp:$exp\n";
    exit;
  }
  return $out;
}

# no action
run_test('', '', '{"error_type":"eventdb","error_message":"Action is not specified"}');

# wrong action
run_test('xx', '', '{"error_type":"eventdb","error_message":"Unknown action: xx"}');

# wrong number of parameters
run_test('login 1',     '', '{"error_type":"login","error_message":"wrong number of arguments, should be 0"}');
run_test('my_info 1', '', '{"error_type":"my_info","error_message":"wrong number of arguments, should be 0"}');
run_test('logout 1',    '', '{"error_type":"logout","error_message":"wrong number of arguments, should be 0"}');
run_test('set_alias',   '', '{"error_type":"set_alias","error_message":"wrong number of arguments, should be 1"}');

# empty db:
run_test('my_info', "",  '{"identity": "anon", "alias": "anon", "level": "anon", "session": "", "stime": 1234567890}');
run_test('my_info', "x", '{"error_type": "jsondb", "error_message":"user.db: No such file or directory"}');

# login with bad token (connection to loginza needed)
#run_test('login', 'xx', '{"error_type":"token_validation","error_message":"Invalid token value."}');

# login test users (vk, fb, lj)
# first user gets admin rights
my $o1 = run_test('login', '382512edfa7149b79b910cf6227e3e16', '{"identity": "http://test.livejournal.com/", "provider": "lj", "full_name": "test", "alias": "test@lj", "level": "admin", "session": "-", "stime": 1234567890}');
my $o2 = run_test('login', '7202c11c442dbd1e7c7f9c33e2ee61d9', '{"identity": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "provider": "fb", "full_name": "Test User", "alias": "TestUser@fb", "level": "norm", "session": "-", "stime": 1234567890}');
my $o3 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', '{"identity": "http://vk.com/id000000000", "provider": "vk", "full_name": "Test User", "alias": "TestUser@vk", "level": "norm", "session": "-", "stime": 1234567890}');

# login again
my $o4 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', '{"identity": "http://vk.com/id000000000", "provider": "vk", "full_name": "Test User", "alias": "TestUser@vk", "level": "norm", "session": "-", "stime": 1234567890}');

# my_info  (o3 session is not valid)
run_test('my_info', $o1->{session}, '{"identity": "http://test.livejournal.com/", "provider": "lj", "full_name": "test", "alias": "test@lj", "level": "admin", "session": "-", "stime": 1234567890}');
run_test('my_info', $o2->{session}, '{"identity": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "provider": "fb", "full_name": "Test User", "alias": "TestUser@fb", "level": "norm", "session": "-", "stime": 1234567890}');
run_test('my_info', $o3->{session}, '{"error_type":"my_info","error_message":"authentication error"}');
run_test('my_info', $o4->{session}, '{"identity": "http://vk.com/id000000000", "provider": "vk", "full_name": "Test User", "alias": "TestUser@vk", "level": "norm", "session": "-", "stime": 1234567890}');
run_test('my_info', "",  '{"identity": "anon", "alias": "anon", "level": "anon", "session": "", "stime": 1234567890}');

# logout
run_test('logout', $o2->{session}, '{"identity": "anon", "alias": "anon", "level": "anon", "session": "", "stime": 1234567890}');
run_test('my_info', $o2->{session}, '{"error_type":"my_info","error_message":"authentication error"}');

# set alias
run_test('set_alias s', $o1->{session}, '{"error_type":"set_alias","error_message":"too short alias"}');
run_test('set_alias sssssssssssssssssssss', $o1->{session}, '{"error_type":"set_alias","error_message":"too long alias"}');
run_test('set_alias s@s', $o1->{session}, '{"error_type":"set_alias","error_message":"only letters, numbers and _ are allowed in alias"}');
run_test('set_alias sla', $o1->{session}, '{"identity": "http://test.livejournal.com/", "provider": "lj", "full_name": "test", "alias": "sla", "level": "admin", "session": "-", "stime": 1234567890}');

# aliases are unique
run_test('set_alias sla', $o4->{session}, '{"error_type":"set_alias","error_message":"alias exists"}');

# logout
run_test('my_info', $o1->{session}, '{"identity": "http://test.livejournal.com/", "provider": "lj", "full_name": "test", "alias": "sla", "level": "admin", "session": "-", "stime": 1234567890}');
run_test('logout', $o1->{session}, '{"identity": "anon", "alias": "anon", "level": "anon", "session": "", "stime": 1234567890}');
run_test('my_info', $o1->{session}, '{"error_type":"my_info","error_message":"authentication error"}');
