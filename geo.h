#ifndef GEO_H
#define GEO_H

#include "dbs.h"
#include "actions.h"

#ifdef MCCME
#define GEO_FDIR "/home/slazav/CH/gps"
#else
#define GEO_FDIR "./gps"
#endif

/* limits: geodata filename size, geodata file size */
#define GEO_MAX_FNAME 32
#define GEO_MAX_FSIZE 100000

extern action_func do_geo_create, do_geo_delete, do_geo_replace,
                   do_geo_edit, do_geo_show, do_geo_list;
#endif
