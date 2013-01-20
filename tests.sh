#!/bin/sh -u

mkdir -p -- ./dbs
rm -f -- ./dbs/*

check(){
  local exp_err=$1; shift;
  local msg=$1; shift;

  if [ "$exp_err" = 1 ]; then
    echo "== Expected Error == $msg =="
  else
    echo "== Expected OK == $msg =="
  fi

  ./eventdb "$@" 2>&1
  local ret=$?

  if [ "$exp_err" = 1 -a "$ret"  = 0 ] ||
     [ "$exp_err" = 0 -a "$ret" != 0 ]; then
    echo "WRONG RESPONSE!"
    exit
  fi
}

echo "======== Create User ========"

check 1 "01 only root can be created" "" "" user_add sla  123
check 1 "02 short password"           "" "" root_add 123
check 0 "02 root created"             "" "" root_add 1231
check 1 "03 permission denied"        "" "" user_add sla  1231
check 1 "04 permission denied"        "" "" user_add root 1231
check 1 "05 permission denied"  "sla" "1231" user_add root 1231
check 1 "06 user exists"       "root" "1231" user_add root 1231
check 0 "07 sla created"       "root" "1231" user_add sla 1231
check 1 "08 empty user name"   "root" "1231" user_add "" ""
check 1 "09 wrong user name"   "root" "1231" user_add "sla.sla" "1"

echo "======== Check User ========"
check 0 "10 root ok"             "root" "1231" user_check
check 1 "11 pwd"                 "root" "12314" user_check
check 1 "12 pwd"                 "sla1" "12314" user_check
check 0 "13 sla ok"              "sla"  "1231" user_check

echo "======== Modify User ========"

check 1 "20 permission denied"          "sla" "1231" user_off sla
check 0 "21 sla deactivated"           "root" "1231" user_off sla
check 1 "22 root can't be deactivated" "root" "1231" user_off root
check 1 "23 sla deactivated"            "sla" "1231" user_chpwd sla 234
check 1 "24 deactivated!"               "sla" "1231" user_check
check 0 "25 sla activated"             "root" "1231" user_on sla
check 1 "26 sla chpwd"                  "sla" "1231" user_chpwd sla 234
check 1 "26 short pwd"                  "sla" "1231" user_mypwd 234
check 0 "26 sla mypwd"                  "sla" "1231" user_mypwd 2342
check 1 "27 permission denied"          "sla" "2342" user_chpwd root 2342
check 0 "28 sla chpwd by root"          "root" "1231" user_chpwd sla 1234
check 0 "29 sla ok"                     "sla" "1234" user_check

echo "======== levels ========"

check 1 "root: sla lvl -> aaa"      "root" "1231" user_chlvl sla aaa
check 0 "root: sla lvl -> 99"       "root" "1231" user_chlvl sla 99
check 0 "sla: sla1 created"          "sla" "1234" user_add sla1 1231
check 0 "sla: sla1 chpwd"            "sla" "1234" user_chpwd sla1 234a
check 1 "sla: root chpwd"            "sla" "1234" user_chpwd root 234a
check 0 "sla: sla1 lvl -> 99"        "sla" "1234" user_chlvl sla1 99
check 1 "sla: root lvl -> 99"        "sla" "1234" user_chlvl root 99
check 0 "sla: sla lvl -> 98"         "sla" "1234" user_chlvl sla 98
check 1 "permission denied"          "sla" "1234" l sla1 user_edit1
check 1 "permission denied"         "sla1" "1231" group_del sla  user_edit
check 1 "sla: user_del"              "sla" "1234" user_del sla1
check 1 "sla1: user_del"            "sla1" "1231" user_del sla
check 0 "root: sla1 deleted"        "root" "1231" user_del sla1

echo "======== Events ========"
check 1 "new event - bad args" "sla" "1234" event_new "title\ntitle" "<body1>\nbody2"\
   "people" "route" "20121201" "20121203"
check 1 "new event - bad date1" "sla" "1234" event_new "title\ntitle" "<body1>\nbody2"\
   "people" "route" "b" "20121203" "100"
check 1 "new event - bad date2" "sla" "1234" event_new "title\ntitle" "<body1>\nbody2"\
   "people" "route" "20121201" "a" "100"
check 1 "new event - bad tags" "sla" "1234" event_new "title\ntitle" "<body1>\nbody2"\
   "people" "route" "20121201" "20121203" "a,b"

check 0 "new event by sla" "sla" "1234" event_new "title\ntitle1" "<body1>
body2" "people" "route" "20121201" "20121203" "1231,124,125,126"
check 0 "new event by sla" "sla" "1234" event_new "title\ntitle2" "<body1>
body2" "people" "route" "20121205" "20121207" "1231,124,125,126"
check 0 "print event" "" "" event_print "1"

check 0 "edit event by sla" "sla" "1234" event_put 1 "title\ntitle1e" "<body1_n>
body2" "people" "route" "20121201" "20121205" "1231,222"
check 0 "print event" "" "" event_print "1"

check 1 "del event 3" "sla" "1234" event_del "3"
check 1 "del event 2 - perm" "" "" event_del "2"
check 0 "del event 2" "sla" "1234" event_del "2"

echo "======== Search ========"

check 0 "search" "" "" event_search "tit" "" "" "" "20121206" "-1" "124"


