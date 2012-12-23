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
check 0 "02 root created"             "" "" root_add 123
check 1 "03 permission denied"        "" "" user_add sla  123
check 1 "04 permission denied"        "" "" user_add root 123
check 1 "05 permission denied"  "sla" "123" user_add root 123
check 1 "06 user exists"       "root" "123" user_add root 123
check 0 "07 sla created"       "root" "123" user_add sla 123
check 1 "08 empty user name"   "root" "123" user_add "" ""
check 1 "09 wrong user name"   "root" "123" user_add "sla.sla" "1"

echo "======== Check User ========"
check 0 "10 root ok"             "root" "123" user_check
check 1 "11 pwd"                 "root" "1234" user_check
check 1 "12 pwd"                 "sla1" "1234" user_check
check 0 "13 sla ok"              "sla"  "123" user_check

echo "======== Modify User ========"

check 1 "20 permission denied"          "sla" "123" user_off sla
check 0 "21 sla deactivated"           "root" "123" user_off sla
check 1 "22 root can't be deactivated" "root" "123" user_off root
check 1 "23 sla deactivated"            "sla" "123" user_chpwd sla 234
check 1 "24 deactivated!"               "sla" "123" user_check
check 0 "25 sla activated"             "root" "123" user_on sla
check 0 "26 sla chpwd"                  "sla" "123" user_chpwd sla 234
check 1 "27 permission denied"          "sla" "234" user_chpwd root 234
check 0 "28 sla chpwd by root"         "root" "123" user_chpwd sla 123

echo "======== Groups ========"

check 0 "sla added to user_edit"    "root" "123" group_add sla user_edit
check 1 "sla already in user_edit"  "root" "123" group_add sla user_edit
check 0 "sla added to user_edit1"   "root" "123" group_add sla user_edit1
check 0 "sla1 created"               "sla" "123" user_add sla1 123
check 1 "permission denied"          "sla" "123" user_chpwd sla1 234
check 0 "sla del from user_edit1"    "sla" "123" group_del sla user_edit1
check 0 "sla1 add to user_edit1"     "sla" "123" group_add sla1 user_edit1
check 1 "no group"                   "sla" "123" group_del sla user_edit2
check 1 "permission denied"         "sla1" "123" group_del sla1 user_edit1
check 1 "permission denied"         "sla1" "123" group_del sla  user_edit
check 1 "permission denied"          "sla" "123" user_del sla1
check 1 "permission denied"         "sla1" "123" user_del sla1
check 0 "sla1 deleted"              "root" "123" user_del sla1

