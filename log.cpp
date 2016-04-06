#include <string>
#include <fstream>
#include <cstdio> /* snprintf */

using namespace std;

/* string with current date and time */
string
time_str(){
  char tstr[25];
  time_t lt;
  time (&lt);
  struct tm * t = localtime(&lt);
  snprintf(tstr, sizeof(tstr), "%04d-%02d-%02d %02d:%02d:%02d",
    t->tm_year+1900, t->tm_mon+1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec);
  return string(tstr);
}

// add the message to the log file
void log(const string & fname,
         const string & msg){
  if (fname.length()==0) return;
  ofstream out(fname.c_str(), ios::app);
  out << time_str() << " " << msg << endl;
}

