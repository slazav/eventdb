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
  $res =~ /\"id\": \"([^\"]+)\"/;
  $out->{id}=$1;
  chomp($res);
  if ($res ne $exp){
    print "ERROR:\n";
    print "  res:\n$res\n";
    print "  exp:\n$exp\n";
    exit;
  }
  return $out;
}

# no action
run_test('', '', '{"error_type":"eventdb","error_message":"Action is not specified"}');

# wrong action
run_test('xx', '', '{"error_type":"eventdb","error_message":"Unknown action: xx"}');

# wrong number of parameters
run_test('login 1',   '', '{"error_type":"login","error_message":"wrong number of arguments, should be 0"}');
run_test('my_info 1', '', '{"error_type":"my_info","error_message":"wrong number of arguments, should be 0"}');
run_test('logout 1',  '', '{"error_type":"logout","error_message":"wrong number of arguments, should be 0"}');
run_test('set_alias', '', '{"error_type":"set_alias","error_message":"wrong number of arguments, should be 1"}');

# empty db:
my $r_an = '{"alias": "anon", "level": -1}';
run_test('my_info', "",  $r_an);
run_test('my_info', "x", '{"error_type": "jsondb", "error_message":"user.db: No such file or directory"}');

# login with bad token (connection to loginza needed)
#run_test('login', 'xx', '{"error_type":"token_validation","error_message":"Invalid token value."}');

# login test users (vk, fb, lj)
# first user gets admin rights
my $r1 = '{"id": 1, "faces": [{"id": "http://test.livejournal.com/", "site": "lj", "name": "test"}], "level": 3, "alias": "test", "stime": 1234567890, "session": "-"}';
my $r2 = '{"id": 2, "faces": [{"id": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "site": "fb", "name": "Test User"}], "level": 0, "alias": "TestUser", "stime": 1234567890, "session": "-"}';
# note the auto alias user01
my $r3 = '{"id": 3, "faces": [{"id": "http://vk.com/id000000000", "site": "vk", "name": "Test User"}], "level": 0, "alias": "user01", "stime": 1234567890, "session": "-"}';

my $o1 = run_test('login', '382512edfa7149b79b910cf6227e3e16', $r1);
my $o2 = run_test('login', '7202c11c442dbd1e7c7f9c33e2ee61d9', $r2);
my $o3 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', $r3);

# login again
my $o4 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', $r3);

# my_info  (o3 session is not valid)
run_test('my_info', $o1->{session}, $r1);
run_test('my_info', $o2->{session}, $r2);
run_test('my_info', $o3->{session}, '{"error_type":"my_info","error_message":"authentication error"}');
run_test('my_info', $o4->{session}, $r3);
run_test('my_info', "",  $r_an);

# logout
run_test('logout', $o2->{session}, $r_an);
run_test('my_info', $o2->{session}, '{"error_type":"my_info","error_message":"authentication error"}');

# set alias
run_test('set_alias s', $o1->{session}, '{"error_type":"set_alias","error_message":"too short alias"}');
run_test('set_alias 1234567890123456789012345678901', $o1->{session}, '{"error_type":"set_alias","error_message":"too long alias"}');
run_test('set_alias s@s', $o1->{session}, '{"error_type":"set_alias","error_message":"only letters, numbers and _ are allowed in alias"}');
$r1=~s/\"alias\": \"[^\"]*\"/\"alias\": \"sla\"/;
run_test('set_alias sla', $o1->{session}, $r1);
run_test('set_alias sla', $o4->{session}, '{"error_type":"set_alias","error_message":"alias exists"}');

# set level
run_test("set_level sla 0", $o1->{session}, '{"error_type":"set_level","error_message":"bad level value"}');
run_test("set_level user01 -2", $o1->{session}, '{"error_type":"set_level","error_message":"bad level value"}');
run_test("set_level user01  3", $o1->{session}, '{"error_type":"set_level","error_message":"bad level value"}');

my $r3a=$r3;
$r3a=~s/, \"session\": \"[^\"]*\"//;
$r3a=~s/, \"stime\": \d*//;
$r3a=~s/\"level\": [\d-]*/\"level\": -1/;
run_test("set_level user01 -1", $o1->{session}, $r3a);
$r3a=~s/\"level\": [\d-]*/\"level\": 2/;
run_test("set_level user01  2", $o1->{session}, $r3a);
$r3a=~s/\"level\": [\d-]*/\"level\": 0/;
run_test("set_level user01  0", $o1->{session}, $r3a);

# logout
run_test('my_info', $o1->{session}, $r1);
run_test('logout', $o1->{session}, $r_an);
run_test('my_info', $o1->{session}, '{"error_type":"my_info","error_message":"authentication error"}');

# user list
$o1 = run_test('login', '382512edfa7149b79b910cf6227e3e16', $r1);
run_test('user_list', $o1->{session}, '[{"id": 1, "faces": [{"id": "http://test.livejournal.com/", "site": "lj", "name": "test"}], "level": 3, "alias": "sla"}, {"id": 2, "faces": [{"id": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "site": "fb", "name": "Test User"}], "level": 0, "alias": "TestUser", "level_hints": [-1, 0, 1, 2]}, {"id": 3, "faces": [{"id": "http://vk.com/id000000000", "site": "vk", "name": "Test User"}], "level": 0, "alias": "user01", "level_hints": [-1, 0, 1, 2]}]');
run_test('user_list', '', '{"error_type":"user_list","error_message":"authentication error"}');

#*********************
# join requests

run_test('joinreq_add user01', $o1->{session}, '{}');
run_test('my_info', $o4->{session}, '{"id": 3, "faces": [{"id": "http://vk.com/id000000000", "site": "vk", "name": "Test User"}], "level": 0, "alias": "user01", "stime": 1234567890, "session": "-", "joinreq": [{"id": "http://test.livejournal.com/", "site": "lj", "name": "test"}]}');
run_test('my_info', $o1->{session}, $r1);
run_test('joinreq_delete 0', $o4->{session}, '{}');
run_test('my_info', $o4->{session}, $r3);

run_test('joinreq_add user01', $o1->{session}, '{}');
run_test('joinreq_accept 0', $o4->{session}, '{}');
run_test('my_info', $o4->{session}, '{"id": 3, "faces": [{"id": "http://vk.com/id000000000", "site": "vk", "name": "Test User"}, {"id": "http://test.livejournal.com/", "site": "lj", "name": "test"}], "level": 0, "alias": "user01", "stime": 1234567890, "session": "-"}');
run_test('my_info', $o1->{session}, '{"error_type":"my_info","error_message":"authentication error"}');


