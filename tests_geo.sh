#!/bin/sh -u

mkdir -p -- ./dbs
rm -f -- ./dbs/tracks
mkdir -p -- ./gps
rm -f ./gps/*

. test_lib.sh

check 1 "01 perm"          "" "" geo_create file comm auth 20120102 20120103 123 1,2,3 < geo.c
check 1 "02 no user" "sla@lj" "123" geo_create file comm auth 20120102 20120103 123 1,2,3 < geo.c
check 0 "03 ok-add"  "sla@lj" "" geo_create file comm auth 20120102 20120103 123 1,2,3 < user.c
check 1 "04 exists"  "sla@lj" "" geo_create file comm auth 20120102 20120103 123 1,2,3 < user.c
check 1 "05 not owner"  "sla1@lj" "" geo_edit file comm auth 20120102 20120103 123 1,2,3,4
check 0 "06 edit"       "sla@lj"  "" geo_edit file comm auth 20120102 20120103 123 1,2,3,4
check 1 "07 not exists" "sla@lj"  "" geo_edit file1 comm auth 20120102 20120103 123 1,2,3,4
check 1 "08 replace: not exists" "sla@lj"  "" geo_replace file1 < user.c
check 1 "09 replace: not owner"  "sla1@lj" "" geo_replace file < user.c
check 0 "10 replace: ok"         "sla@lj"  "" geo_replace file < user.c
check 0 "11 show:"               "sla@lj"  "" geo_show file
check 1 "12 not exists"          "sla@lj"  "" geo_show file1
check 1 "13 del:not exists"      "sla@lj"  "" geo_delete file1
check 1 "14 not owner"          "sla1@lj"  "" geo_delete file
check 0 "15 delete"              "sla@lj"  "" geo_delete file
