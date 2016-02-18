#include <iostream>
#include "err.h" // error handling
#include "cfg.h" // read config file
#include "act.h" // actions

/* Main program:
  - expects at least one argument with an action
  - run the action
  - catch exceptions, print json output, close databases etc.
*/

int
main(int argc, char **argv){

  try {
    if (argc<2) throw Err("eventdb") << "Action is not specified";
    std::string action(argv[1]);
    CFG cfg; /* read parameters from the config file */

    if      (action == "login")     do_login(cfg, argc-1, argv+1); 
    else if (action == "logout")    do_logout(cfg, argc-1, argv+1);
    else if (action == "user_info") do_user_info(cfg, argc-1, argv+1);
    else throw Err("eventdb") << "Unknown action";
  }
  catch(Err e){
    std::cout << e.json() << std::endl;
  }
  catch (Exc e){
    std::cout << e.json() << std::endl;
  }
  exit(0);
}
