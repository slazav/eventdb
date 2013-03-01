#!/bin/sh -u

mkdir -p -- ./dbs
rm -f -- ./dbs/links
rm -f -- ./dbs/e2ln
. test_lib.sh

## after test_user and test event there is user sla/1234 and event 1

check 1 "new link - bad args" "sla" "1234" link_create "url" "text" ""
check 1 "new link - bad event" "sla" "1234" link_create 1024 "url" "text" "1,2,3"
check 1 "new link - bad tags" "sla" "1234" link_create 1 "url" "text" "a,b"

check 0 "new link by sla" "sla" "1234" link_create 1 "url1" "text1" "1,2,3"
check 0 "new link by sla" "sla" "1234" link_create 1 "url2" "text2" "4,5,6"
check 0 "print link" "" "" link_show "1"
check 0 "list links" "" "" link_list
check 0 "links for ev 1" "" "" link_list_ev 1
check 0 "links for ev 2" "" "" link_list_ev 2


check 1 "edit: no link" "sla" "1234" link_edit 1024 "url1" "text1" "1,2,3"
check 1 "edit: perm" "somebody" "" link_edit 1 "urlm" "textm" "1,2,3"

check 0 "edit link by sla" "sla" "1234" link_edit 1 "urls" "texts" "11,22,33"

check 0 "print link" "" "" link_show "1"
check 1 "del link 3" "sla" "1234" link_delete "3"
check 1 "del link 2 - perm" "smb" "" link_delete "2"
check 0 "del link 2" "sla" "1234" link_delete "2"

echo "======== Search ========"

check 0 "search" "" "" link_search "" "" "" "" "" "" "" ""


