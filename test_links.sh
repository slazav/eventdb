#!/bin/sh -u

mkdir -p -- ./dbs
rm -f -- ./dbs/links
rm -f -- ./dbs/e2ln
mkdir -p -- ./gps
rm -f ./gps/*
. test_lib.sh


## after test_user and test event there is user sla/1234 and event 1

check 1 "new link - bad args" "sla" "1234" link_create "url" "text" "auth" 0 ""
check 1 "new link - bad event" "sla" "1234" link_create 1024 "url" "text" "auth" 0 "1,2,3"
check 1 "new link - bad tags" "sla" "1234" link_create 1 "url" "text" "auth" 0 "a,b"

check 0 "new link by sla" "sla" "1234" link_create 1 "url1" "text1" "auth" 0 "1,2,3"
check 0 "new link by sla" "sla" "1234" link_create 1 "url2" "text2" "auth" 0 "4,5,6"
check 0 "print link" "" "" link_show "1"
check 0 "list links" "" "" link_list
check 0 "links for ev 1" "" "" link_list_ev 1
check 0 "links for ev 2" "" "" link_list_ev 2


check 1 "edit: no link" "sla" "1234" link_edit 1024 "url1" "text1" "auth" 0 "1,2,3"
check 1 "edit: perm" "somebody" "" link_edit 1 "urlm" "textm" "auth" 0 "1,2,3"

check 0 "edit link by sla" "sla" "1234" link_edit 1 "urls" "texts" "auth" 0 "11,22,33"

check 0 "print link" "" "" link_show "1"
check 1 "del link 3" "sla" "1234" link_delete "3"
check 1 "del link 2 - perm" "smb" "" link_delete "2"
check 0 "del link 2" "sla" "1234" link_delete "2"

echo "======== Search ========"

check 0 "search" "" "" link_search "" "" "" "" ""

echo "======== Local file operations ========"
check 1 "01 perm"          "" ""      link_create 1 file comm auth 1 1,2,3 < link.c
check 1 "02 no user" "sla@lj" "123"   link_create 1 file comm auth 1 1,2,3 < link.c
check 0 "03 ok-add"  "sla@lj" ""      link_create 1 file comm auth 1 1,2,3 < user.c
check 1 "04 exists"  "sla@lj" ""      link_create 1 file comm auth 1 1,2,3 < user.c
check 1 "05 not owner"  "sla1@lj" ""  link_edit 2 file comm auth 1 1,2,3,4
check 0 "06 edit"       "sla@lj"  ""  link_edit 2 file comm auth 1 1,2,3,4
check 1 "07 not exists" "sla@lj"  ""  link_edit 3 file2 comm auth 1 1,2,3,4
check 1 "08 replace: not exists" "sla@lj"  "" link_replace 3 < user.c
check 1 "09 replace: not owner"  "sla1@lj" "" link_replace 2 < user.c
check 0 "10 replace: ok"         "sla@lj"  "" link_replace 2 < user.c
check 0 "11 show:"               "sla@lj"  "" link_show 2
check 1 "12 not exists"          "sla@lj"  "" link_show 3
check 1 "13 del:not exists"      "sla@lj"  "" link_delete 3
check 1 "14 not owner"          "sla1@lj"  "" link_delete 2
check 0 "15 delete"              "sla@lj"  "" link_delete 2

