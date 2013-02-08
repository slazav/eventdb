#!/bin/sh -u

mkdir -p -- ./dbs
rm -f -- ./dbs/users

. test_lib.sh

echo "======== Create User ========"
check 1 "00 wrong action"             "" "" wrong
check 1 "01 too many pars"            "" "" user_add sla  123 234
check 1 "02 not enough pars"          "" "" user_add sla
check 1 "03 only root can be created" "" "" user_add sla  123
check 1 "04 wrong user/passwd"        "sla" "123" user_add sla  123
check 1 "05 wrong user/passwd"        "" "123" user_add sla  123
check 1 "06 short password"           "" "" root_add 123
check 0 "07 root created"             "" "" root_add 1231
check 1 "08 permission denied"        "" "" user_add sla  1231
check 1 "09 permission denied"        "" "" user_add root 1231
check 1 "10 user exists"        "sla" "1231" user_add root 1231
check 1 "11 user exists"       "root" "1231" user_add root 1231
check 0 "12 sla created"       "root" "1231" user_add sla 1231
check 1 "13 empty user name"   "root" "1231" user_add "" 1231
check 1 "14 wrong user name"   "root" "1231" user_add "sla.sla" 1231

echo "======== Check User ========"
check 0 "20 root ok"             "root" "1231"  level_show
check 1 "21 pwd"                 "root" "12314" level_show
check 1 "22 pwd"                 "sla1" "12314" level_show
check 0 "23 sla ok"              "sla"  "1231"  level_show
check 0 "24 noauth ok"           "sla"  ""      level_show
check 0 "24 anon ok"                ""  ""      level_show

echo "======== Modify User ========"

check 1 "30 permission denied"          "sla" "1231" user_off sla
check 0 "31 sla deactivated"           "root" "1231" user_off sla
check 1 "32 root can't be deactivated" "root" "1231" user_off root
check 1 "33 sla deactivated"            "sla" "1231" user_chpwd sla 234
check 1 "34 deactivated!"               "sla" "1231" user_check
check 0 "35 sla activated"             "root" "1231" user_on sla
check 1 "36 sla chpwd"                  "sla" "1231" user_chpwd sla 234
check 1 "37 short pwd"                  "sla" "1231" user_mypwd 234
check 0 "38 sla mypwd"                  "sla" "1231" user_mypwd 2342
check 1 "39 permission denied"          "sla" "2342" user_chpwd root 2342
check 0 "40 sla chpwd by root"         "root" "1231" user_chpwd sla 1234
check 0 "41 sla ok"                     "sla" "1234" level_show

echo "======== levels ========"

check 1 "30 root: sla lvl -> aaa"      "root" "1231" user_level_set sla aaa
check 0 "31 root: sla lvl -> 99"       "root" "1231" user_level_set sla 99
check 1 "32 set lvl 100"               "root" "1231" user_level_set sla 100
check 0 "33 sla: sla1 created"          "sla" "1234" user_add sla1 1231
check 0 "34 sla: sla1 chpwd"            "sla" "1234" user_chpwd sla1 234a
check 1 "35 sla: root chpwd"            "sla" "1234" user_chpwd root 234a
check 0 "36 sla: sla1 lvl -> 99"        "sla" "1234" user_level_set sla1 99
check 1 "37 sla: root lvl -> 99"        "sla" "1234" user_level_set root 99
check 0 "38 sla: sla lvl -> 98"         "sla" "1234" user_level_set sla 98
check 1 "39 permission denied"          "sla" "1234" l sla1 user_edit1
check 1 "40 permission denied"         "sla1" "1231" group_del sla  user_edit
check 1 "41 sla: user_del"              "sla" "1234" user_del sla1
check 1 "42 sla1: user_del"            "sla1" "1231" user_del sla
check 0 "43 root: sla1 deleted"        "root" "1231" user_del sla1

