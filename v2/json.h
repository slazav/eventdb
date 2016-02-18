#ifndef JSON_H
#define JSON_H

#include <string>
#include <jansson.h>

/*
  Useful functions for json format handling.
*/

json_t * j_mkobj();                    // create json object
std::string j_dumpstr(json_t *root);   // dump json to string
json_t * j_loadstr(const std::string & str);  // load json from string
json_t * j_loadfile(const std::string & file); // load json from file

/* Check that root is an object, get one field. */
/* If def=NULL then the non-exist field causes error.*/
std::string j_getstr(json_t *root,
   const std::string & key, const char *def = NULL);

/* put key:pair value into json object */
void j_putstr(json_t *root,
   const std::string & key, const std::string & val);

#endif
