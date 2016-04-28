#include <iostream>
#include "err.h" // error handling
#include "cfg.h" // read config file
#include "log.h" //
#include "actions.h" // actions
#include <sys/stat.h> // umask

#include "jsonxx/jsonxx.h"
#include "jsondb/jsondb.h"

#ifndef CFG_FILE
#define CFG_FILE "./config.json"
#endif

CFG cfg;

template <typename Res>
void process_result(const Res & e){
  log(cfg.logfile, e.text());
  std::cout << e.json() << std::endl;
}

/* Dump user databases */
int
main(){
  umask(066);
  try {
    cfg.read(CFG_FILE); /* read parameters from the config file */
    local_dump_db(cfg, 0, NULL);
  }
  catch(Err e)        { process_result(e); }
  catch(Json::Err e)  { process_result(e); }
  catch(JsonDB::Err e){ process_result(e); }
  catch (Exc e)       { process_result(e); }
  exit(0);
}
