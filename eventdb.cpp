#include <iostream>
#include "err.h" // error handling
#include "cfg.h" // read config file
#include "log.h" //
#include "actions.h" // actions

#include "jsonxx/jsonxx.h"
#include "jsondb/jsondb.h"

#include <sys/stat.h> // umask

#ifndef CFG_FILE
#define CFG_FILE "./config.json"
#endif

CFG cfg;

template <typename Res>
void process_result(const Res & e){
  log(cfg.logfile, e.text());
  std::cout << e.json() << std::endl;
}

/* Main program:
  - expects at least one argument - action
  - run the action
  - catch exceptions, print json output, close databases etc.
*/
int
main(int argc, char **argv){

  try {
    if (argc<2) throw Err("eventdb") << "Action is not specified";
    std::string action(argv[1]);
    cfg.read(CFG_FILE); /* read parameters from the config file */

    umask(066);

    // log the action
    std::string l("action:");
    for (int i=0;i<argc;i++) l+=std::string(" ") + argv[i];
    log(cfg.logfile, l);

    if      (action == "login")     do_login(cfg, argc-1, argv+1); 
    else if (action == "logout")    do_logout(cfg, argc-1, argv+1);
    else if (action == "my_info")   do_my_info(cfg, argc-1, argv+1);
    else if (action == "set_alias") do_set_alias(cfg, argc-1, argv+1);
    else if (action == "set_level") do_set_level(cfg, argc-1, argv+1);
    else if (action == "user_list") do_user_list(cfg, argc-1, argv+1);
    else if (action == "joinreq_add") do_joinreq_add(cfg, argc-1, argv+1);
    else if (action == "joinreq_delete") do_joinreq_delete(cfg, argc-1, argv+1);
    else if (action == "joinreq_accept") do_joinreq_accept(cfg, argc-1, argv+1);
    else throw Err("eventdb") << "Unknown action: " << action;
    throw Exc() << "{}";
  }
  catch(Err e)        { process_result(e); }
  catch(Json::Err e)  { process_result(e); }
  catch(JsonDB::Err e){ process_result(e); }
  catch (Exc e)       { process_result(e); }
  exit(0);
}
