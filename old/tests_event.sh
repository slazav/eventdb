#!/bin/sh -u

mkdir -p -- ./dbs
rm -f -- ./dbs/events
rm -f -- ./dbs/d2ev
. test_lib.sh

echo "======== Events ========"
check 1 "new event - bad args" "sla" "1234" event_create "title\ntitle" "<body1>\nbody2"\
   "people" "route" "20121201" "20121203"
check 1 "new event - bad date1" "sla" "1234" event_create "title\ntitle" "<body1>\nbody2"\
   "people" "route" "b" "20121203" "100"
check 1 "new event - bad date2" "sla" "1234" event_create "title\ntitle" "<body1>\nbody2"\
   "people" "route" "20121201" "a" "100"
check 1 "new event - bad tags" "sla" "1234" event_create "title\ntitle" "<body1>\nbody2"\
   "people" "route" "20121201" "20121203" "a,b"

check 0 "new event by sla" "sla" "1234" event_create\
  "title\ntitle1" "<body1> body2" "people" "route" "20121201" "20121203" "1231,124,125,126"
check 0 "new event by sla" "sla" "1234" event_create\
  "title\ntitle2" "<body1> body2" "people" "route" "20121205" "20121207" "1231,124,125,126"
check 0 "print event" "" "" event_show "1"

check 1 "edit: no event" "sla" "1234" event_edit 3\
  "title\ntitle1e" "<body1_n>
body2" "people" "route" "20121201" "20121205" "1231,222"
check 0 "edit event by sla" "sla" "1234" event_edit 1\
  "title\ntitle1e" "<body1_n>
body2" "people" "route" "20121201" "20121205" "1231,222"
check 0 "print event" "" "" event_show "1"
check 0 "list event" "" "" event_list
check 1 "del event 3" "sla" "1234" event_delete "3"
check 1 "del event 2 - perm" "" "" event_delete "2"
check 0 "del event 2" "sla" "1234" event_delete "2"

echo "======== Search ========"

check 0 "search" "" "" event_search "" "" "" "" "" "" "" ""


