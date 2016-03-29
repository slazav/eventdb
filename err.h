#ifndef ERR_H
#define ERR_H

#include <string>
#include <sstream>

/*
  Classes for errors exceptions.
  Example:

  try {
    if (SomethingWrong) throw Err("WRONG1") << "Something is wrong";
    if (SomethingElse)  throw Err("WRONG2") << "Something else is wrong";
    throw Exc << "OK!";
  }
  catch (Err e){
    cerr << e.str()  << endl;
    cout << e.json() << endl;
    if (e.type() == "WRONG1") exit(1):
    else exit(2);
  }
  catch (Exc e){
    cout << e.json() << endl;
  }
*/

class Err{
  static std::string t;
  std::ostringstream s;

public:
  Err(const std::string & type = std::string()){
    if (type.length()) t=type;
  }
  Err(const Err & o) { s << o.s.str(); }

  std::string str()  const { return s.str(); }
  std::string type() const { return t; }
  std::string json() const {
    return std::string("{\"error_type\":\"") + t + "\"," +
                        "\"error_message\":\"" + s.str() + "\"}"; }
  template <typename T>
  Err & operator<<(const T & o){ s << o; return *this; }
};

/* Other exceptions: just print a text message */
class Exc{
  std::ostringstream s;
public:
  Exc() {}
  Exc(const Exc & o) { s << o.s.str(); }
  std::string json()  const { return s.str(); }
  template <typename T>
  Exc & operator<<(const T & o){ s << o; return *this; }
};

#endif
