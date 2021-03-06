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
  my $res  = `printf "%s" '$sec' | ./eventdb $args`;
  my $out;
  $res =~ s/\"stime\": (\d+)/\"stime\": 1234567890/;
  $res =~ s/\"ctime\": (\d+)/\"ctime\": 1234567890/;
  $res =~ s/\"mtime\": (\d+)/\"mtime\": 1234567890/;
  $out->{stime}=$1;
  $res =~ s/\"session\": \"([^\"]+)\"/\"session\": \"ssss\"/;
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
my $r_an = '{"level": -1}';
run_test('my_info', "",  $r_an);
run_test('my_info', "x", $r_an);

# login with bad token (connection to loginza needed)
#run_test('login', 'xx', '{"error_type":"token_validation","error_message":"Invalid token value."}');

# test faces
my $LJF = '{"id": "http://test.livejournal.com/", "site": "lj", "name": "test"}';
my $FBF = '{"id": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "site": "fb", "name": "Test User"}';
my $VKF = '{"id": "http://vk.com/id000000000", "site": "vk", "name": "Test User"}';
# test sid's
my $LJS = $LJF; $LJS =~ s/.*\"id\": (\"[^\"]*\").*/\"sid\": $1/;
my $FBS = $FBF; $FBS =~ s/.*\"id\": (\"[^\"]*\").*/\"sid\": $1/;
my $VKS = $VKF; $VKS =~ s/.*\"id\": (\"[^\"]*\").*/\"sid\": $1/;
my $ST = '"stime": 1234567890, "session": "ssss"'; # these non-reproducable values are converted by run_test()

my $r1 = '{"id": 1, "faces": ['.$LJF.'], "level": 3, "alias": "test", '.$LJS.', '.$ST.'}';
my $r2 = '{"id": 2, "faces": ['.$FBF.'], "level": 0, "alias": "TestUser", '.$FBS.', '.$ST.'}';
# note the auto alias user01
my $r3 = '{"id": 3, "faces": ['.$VKF.'], "level": 0, "alias": "user01", '.$VKS.', '.$ST.'}';

# login test users (vk, fb, lj)
# first user gets admin rights
my $o1 = run_test('login', '382512edfa7149b79b910cf6227e3e16', $r1);
my $o2 = run_test('login', '7202c11c442dbd1e7c7f9c33e2ee61d9', $r2);
my $o3 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', $r3);

# login again
my $o4 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', $r3);

# my_info  (o3 session is not valid)
run_test('my_info', $o1->{session}, $r1);
run_test('my_info', $o2->{session}, $r2);
run_test('my_info', $o3->{session}, $r_an);
run_test('my_info', $o4->{session}, $r3);
run_test('my_info', "",  $r_an);

# logout
run_test('logout', $o2->{session}, $r_an);
run_test('my_info', $o2->{session}, $r_an);

# set alias
run_test('set_alias s', $o1->{session}, '{"error_type":"set_alias","error_message":"too short name"}');
run_test('set_alias 1234567890123456789012345678901', $o1->{session}, '{"error_type":"set_alias","error_message":"too long name"}');
run_test('set_alias s@s', $o1->{session}, '{"error_type":"set_alias","error_message":"only letters, numbers and _ are allowed in name"}');
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
run_test('my_info', $o1->{session}, $r_an);

# user list
$o1 = run_test('login', '382512edfa7149b79b910cf6227e3e16', $r1);
run_test('user_list', $o1->{session}, '[{"id": 1, "faces": ['.$LJF.'], "level": 3, "alias": "sla"},'.
                                      ' {"id": 2, "faces": ['.$FBF.'], "level": 0, "alias": "TestUser", "level_hints": [-1, 0, 1, 2]},'.
                                      ' {"id": 3, "faces": ['.$VKF.'], "level": 0, "alias": "user01", "level_hints": [-1, 0, 1, 2]}]');
run_test('user_list', '', '{"error_type":"user_list","error_message":"authentication error"}');

#*********************
# join requests

# login again
$o1 = run_test('login', '382512edfa7149b79b910cf6227e3e16', $r1);
$o2 = run_test('login', '7202c11c442dbd1e7c7f9c33e2ee61d9', $r2);
$o3 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', $r3);

# add - delete
run_test('joinreq_add user01', $o1->{session}, '{}');
run_test('my_info', $o3->{session}, '{"id": 3, "faces": ['.$VKF.'], "level": 0, "alias": "user01", '.$VKS.', '.$ST.', "joinreq": ['.$LJF.']}');
run_test('my_info', $o1->{session}, $r1);

run_test('joinreq_add user01', $o1->{session}, '{}');
run_test('joinreq_delete 0', $o3->{session}, '{"id": 3, "faces": ['.$VKF.'], "level": 0, "alias": "user01", '.$VKS.', '.$ST.'}');
run_test('my_info', $o1->{session}, $r1);
run_test('my_info', $o3->{session}, $r3);

# 1->3
run_test('joinreq_add user01', $o1->{session}, '{}');
run_test('joinreq_accept 0', $o3->{session}, '{"id": 3, "faces": ['.$VKF.', '.$LJF.'], "level": 3, "alias": "user01", '.$VKS.', '.$ST.'}');
run_test('my_info', $o3->{session}, '{"id": 3, "faces": ['.$VKF.', '.$LJF.'], "level": 3, "alias": "user01", '.$VKS.', '.$ST.'}');

# user o1 does not exists any more
run_test('my_info', $o1->{session}, $r_an);

# 3->2
run_test('joinreq_add TestUser', $o3->{session}, '{}');
run_test('my_info', $o2->{session}, '{"id": 2, "faces": ['.$FBF.'], "level": 0, "alias": "TestUser", '.$FBS.', '.$ST.', "joinreq": ['.$VKF.', '.$LJF.']}');

#accept both
run_test('joinreq_accept 1', $o2->{session}, '{"id": 2, "faces": ['.$FBF.', '.$LJF.'], "level": 3, "alias": "TestUser", '.$FBS.', '.$ST.', "joinreq": ['.$VKF.']}');
run_test('joinreq_accept 0', $o2->{session}, '{"id": 2, "faces": ['.$FBF.', '.$LJF.', '.$VKF.'], "level": 3, "alias": "TestUser", '.$FBS.', '.$ST.'}');

run_test('my_info', $o2->{session}, '{"id": 2, "faces": ['.$FBF.', '.$LJF.', '.$VKF.'], "level": 3, "alias": "TestUser", '.$FBS.', '.$ST.'}');

#*********************
# write/read data

# cleanup data folders
`rm -fr   -- ./data ./logs ./files`;
`mkdir -p -- ./data ./logs ./files`;

# login again
$r1 = '{"id": 1, "faces": ['.$LJF.'], "level": 3, "alias": "test", '.$LJS.', '.$ST.'}';
$r2 = '{"id": 2, "faces": ['.$FBF.'], "level": 0, "alias": "TestUser", '.$FBS.', '.$ST.'}';
$r3 = '{"id": 3, "faces": ['.$VKF.'], "level": 0, "alias": "user01", '.$VKS.', '.$ST.'}';
$o1 = run_test('login', '382512edfa7149b79b910cf6227e3e16', $r1);
$o2 = run_test('login', '7202c11c442dbd1e7c7f9c33e2ee61d9', $r2);
$o3 = run_test('login', '6222d12c54a233deae789c3ce22eb1d9', $r3);

# wrong number of arguments:
run_test('write',  $o2->{session}, '{"error_type":"write","error_message":"wrong number of arguments, should be 1"}' );
run_test('read',   $o2->{session}, '{"error_type":"read","error_message":"wrong number of arguments, should be 2"}' );
run_test('read_arc',   $o2->{session}, '{"error_type":"read_arc","error_message":"wrong number of arguments, should be 2"}' );
run_test('search', $o2->{session}, '{"error_type":"search","error_message":"wrong number of arguments, should be 1"}' );

# o2 can't create databases
run_test('write db0', $o2->{session}."\n{}", '{"error_type": "jsondb", "error_message":"db0.db: No such file or directory"}' );

run_test('write db0', "$o1->{session}\n".'{"data":[0,2]}', '{"data": [0, 2], "id": 1, "mtime": 1234567890, "muser": "http://test.livejournal.com/", "ctime": 1234567890, "cuser": "http://test.livejournal.com/", "prev": -1}' );
run_test('write db0', "$o2->{session}\n".'{"data":[2,3],"id":-1}', '{"data": [2, 3], "id": 2, "mtime": 1234567890, "muser": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "ctime": 1234567890, "cuser": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "prev": -1}' );
run_test('write db0', "$o1->{session}\n".'{"id":5}',   '{"error_type":"write","error_message":"bad id"}' );
run_test('write db0', "$o1->{session}\n".'{"id":"a"}', '{"error_type": "jsonxx", "error_message":"can\'t cast to integer"}');
run_test('read db0 2',  $o1->{session}, '{"data": [2, 3], "id": 2, "mtime": 1234567890, "muser": "TestUser", "ctime": 1234567890, "cuser": "TestUser", "prev": -1}' );
run_test('read db0 4',  $o1->{session}, '{"error_type": "jsondb", "error_message":"db0.db: DB_NOTFOUND: No matching key/data pair found"}' );

# now object 1 created by superuser, object 2 created by normal user
# try to replace:

run_test('write db0', "$o2->{session}\n".'{"data":[4,5],"id":1}', '{"error_type":"write","error_message":"not enough permissions to edit"}' );
run_test('write db0', "$o2->{session}\n".'{"data":[5,6],"id":2}', '{"data": [5, 6], "id": 2, "mtime": 1234567890, "muser": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "ctime": 1234567890, "cuser": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "prev": 1}' );
run_test('write db0', "$o1->{session}\n".'{"data":[6,7],"id":2}', '{"data": [6, 7], "id": 2, "mtime": 1234567890, "muser": "http://test.livejournal.com/", "ctime": 1234567890, "cuser": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "prev": 2}' );
run_test('read db0 2',"", '{"data": [6, 7], "id": 2, "mtime": 1234567890, "muser": "test", "ctime": 1234567890, "cuser": "TestUser", "prev": 2}' );

# read archive objects:
run_test('read_arc db0 1',"", '{"data": [2, 3], "id": 1, "mtime": 1234567890, "muser": "TestUser", "ctime": 1234567890, "cuser": "TestUser", "prev": -1}' );
run_test('read_arc db0 2',"", '{"data": [5, 6], "id": 2, "mtime": 1234567890, "muser": "TestUser", "ctime": 1234567890, "cuser": "TestUser", "prev": 1}' );
run_test('read_arc db0 4',"", '{"error_type": "jsondb", "error_message":"db0.arc.db: DB_NOTFOUND: No matching key/data pair found"}' );

# dates
run_test('write db0', "$o2->{session}\n".'{"data":[5,6],"date1":"2016/01/02","date2":"2016/02/12","id":2}',
  '{"data": [5, 6], "date1": "2016/01/02", "date2": "2016/02/12", "id": 2, "mtime": 1234567890, "muser": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "ctime": 1234567890, "cuser": "https://www.facebook.com/app_scoped_user_id/000000000000000/", "prev": 3, "date_key": "2016/0"}' );
