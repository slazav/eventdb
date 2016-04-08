#include <iostream>
#include <iomanip>
#include <malloc.h>
#include <set>

#include "jsondb.h"

#define ASSERT_OP(lhs, rhs, op, m) \
	do { \
		if(!((lhs) op (rhs))) { \
			std::cerr << std::boolalpha; \
			std::cerr << __FILE__ << '[' << __LINE__ << "]: ERROR: " << (m) << std::endl; \
			std::cerr << "\ttest:   " << #lhs << ' ' << #op << ' ' << #rhs << std::endl; \
			std::cerr << "\tlhs: " << (lhs) << "." << std::endl; \
			std::cerr << "\trhs: " << (rhs) << "." << std::endl; \
			return 1; \
		} \
	} while(0)
#define ASSERT_EQ(lhs, rhs, m) ASSERT_OP(lhs, rhs, ==, m)
#define ASSERT_NE(lhs, rhs, m) ASSERT_OP(lhs, rhs, !=, m)
#define ASSERT_TRUE(p, m) ASSERT_OP(p, true, ==, m)
#define ASSERT_FALSE(p, m) ASSERT_OP(p, true, !=, m)

int
main() {
  try {

    const char * c1 = "{\"names\": [\"a\", \"b\", \"c\"], "
                      "\"unames\": [\"a1\", \"b1\"], "
                      "\"value\": 10}";
    const char * c2 = "{\"names\": [{\"id\": \"d\", \"value\": 0}, \"d\", \"c\"], "
                      "\"unames\": \"a2\", "
                      "\"value\": 10}";
    const char * c3 = "{\"names\": \"a\", "
                      "\"unames\": {\"id\": \"a3\"}, "
                      "\"value\": 30}";
    const char * c4 = "{\"unames\": [\"a4\"], "
                      "\"value\": 40}";
    const char * c5 = "{\"names\": \"c\", "
                      "\"unames\": [\"a5\"], "
                      "\"value\": 50}";

    // first try the DB with integer keys
    bool intkeys = true;
    {
      JsonDB db("test1", intkeys, DB_TRUNCATE | DB_CREATE); // open DB
      db.secondary_open("names",  true);  // duplicated entries allowed
      db.secondary_open("unames", false); // duplicated entries not allowed

      ASSERT_TRUE(db.is_empty(), "db is not empty!");
      Json j1 = Json::load_string(c1);
      Json j2 = Json::load_string(c2);
      j1.set("id",1);
      j2.set("id",2);
      db.put(j1);
      db.put(j2);
    }
    // add more entries
    {
      JsonDB db("test1", intkeys); // open DB
      db.secondary_open("names",  true);  // duplicated entries allowed
      db.secondary_open("unames", false); // duplicated entries not allowed

      ASSERT_FALSE(db.is_empty(), "db is empty!");
      Json j3 = Json::load_string(c3);
      j3.set("id",4);
      db.put(j3);

      try{
        j3.set("id","10");
        db.put(j3);
        ASSERT_TRUE(false, "string key");
      } catch(JsonDB::Err e){};

      try{
        j3.set("id",4);
        db.put(j3, false);
        ASSERT_TRUE(false, "overwriting");
      } catch(JsonDB::Err e){};

      Json j4 = Json::load_string(c4);
      Json j5 = Json::load_string(c5);
      j4.set("id",-1);
      j5.set("id",-1);
      db.put(j4);
      db.put(j5);

      try{
        Json j = Json::load_string("{\"names\": \"1\", \"unames\": 2, \"id\":-1}");
        db.put(j);
        ASSERT_TRUE(false, "string or array expected for unames");
      } catch(JsonDB::Err e){};

      try{
        Json j = Json::load_string("{\"names\": \"d\", \"unames\": \"a1\", \"id\":-1}");
        db.put(j);
        ASSERT_TRUE(false, "non-unique key");
      } catch(JsonDB::Err e){};
    }

    const char *r1 = "{\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10, \"id\": 1}";
    const char *r2 = "{\"names\": [{\"id\": \"d\", \"value\": 0}, \"d\", \"c\"], \"unames\": \"a2\", \"value\": 10, \"id\": 2}";
    const char *r3 = "{\"names\": \"a\", \"unames\": {\"id\": \"a3\"}, \"value\": 30, \"id\": 4}";
    const char *r4 = "{\"unames\": [\"a4\"], \"value\": 40, \"id\": 5}";
    const char *r5 = "{\"names\": \"c\", \"unames\": [\"a5\"], \"value\": 50, \"id\": 6}";

    // check entries
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      ASSERT_FALSE(db.is_empty(), "db is empty!");
      ASSERT_TRUE(db.exists(1), "exist key1");
      ASSERT_TRUE(db.exists(2), "exist key2");
      ASSERT_FALSE(db.exists(3), "exist key3");
      ASSERT_TRUE(db.exists(4), "exist key4");


      ASSERT_EQ(db.get_str(1), r1, "get_str1");
      ASSERT_EQ(db.get_str(2), r2, "get_str2");
      ASSERT_EQ(db.get_str(4), r3, "get_str3");
      ASSERT_EQ(db.get_str(5), r4, "get_str4");
      ASSERT_EQ(db.get_str(6), r5, "get_str5");
      ASSERT_NE(db.get_str(1), r2, "get_str3");
      ASSERT_EQ(db.get_last_id(), 6, "last_id");

      std::string res0 = db.get_all().save_string(JSON_PRESERVE_ORDER);
      std::string exp0;
      exp0 = exp0 +  "[" + r1
                  + ", " + r2
                  + ", " + r3
                  + ", " + r4
                  + ", " + r5 + "]";

      ASSERT_EQ(res0, exp0, "get_all");

    }
    // check secondary dbs
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      db.secondary_open("names",  true);  // duplicated entries allowed
      db.secondary_open("unames", false); // duplicated entries not allowed

      std::string resa  = (db.secondary_get("names", "a")).save_string(JSON_PRESERVE_ORDER);
      std::string resb  = (db.secondary_get("names", "b")).save_string(JSON_PRESERVE_ORDER);
      std::string resc  = (db.secondary_get("names", "c")).save_string(JSON_PRESERVE_ORDER);
      std::string resd  = (db.secondary_get("names", "d")).save_string(JSON_PRESERVE_ORDER);
      std::string resa2 = (db.secondary_get("unames", "a2")).save_string(JSON_PRESERVE_ORDER);

      std::string expa,expb,expc,expd;
      expa = expa + "[" + r1 + ", " + r3 + "]";
      expb = expb + "[" + r1 + "]";
      expc = expc + "[" + r1 + ", " + r2 + ", " + r5 + "]";
      expd = expd + "[" + r2 + "]";

      ASSERT_EQ(resa, expa, "find names == a");
      ASSERT_EQ(resb, expb, "find names == b");
      ASSERT_EQ(resc, expc, "find names == c");
      ASSERT_EQ(resd, expd, "find names == d");
      ASSERT_EQ(resa2,expd, "find unames == a2");

      ASSERT_TRUE(db.secondary_exists("names",   "a"), "secondary_exists - a");
      ASSERT_TRUE(db.secondary_exists("unames", "a2"), "secondary_exists - a2");
      ASSERT_FALSE(db.secondary_exists("names",   "z"), "secondary_exists - z");
      ASSERT_FALSE(db.secondary_exists("unames", "z2"), "secondary_exists - z2");
    }

    // add log entry
    {
      JsonDB db("test1", intkeys); // open DB
      db.secondary_open("names",  true);  // duplicated entries allowed
      db.secondary_open("unames", false); // duplicated entries not allowed
      Json j1 = Json::load_string("{\"a\": \"string1\"}");
      Json j2 = Json::load_string("{\"a\": \"string1\"}");
      db.put_time(j1);
      db.put_time(j2);
      json_int_t t1 = j1["id"].as_integer();
      json_int_t t2 = j2["id"].as_integer();
      ASSERT_NE(t1,t2,"log1");
    }

    /*********************************************************************************/
    // repeat the same with string keys
    intkeys = false;
    {
      JsonDB db("test1", intkeys, DB_TRUNCATE | DB_CREATE); // open DB
      db.secondary_open("names",  true);  // duplicated entries allowed
      db.secondary_open("unames", false); // duplicated entries not allowed

      ASSERT_TRUE(db.is_empty(), "db is not empty!");
      Json j1 = Json::load_string(c1);
      Json j2 = Json::load_string(c2);
      j1.set("id","e1");
      j2.set("id","e2");
      db.put(j1);
      db.put(j2);
    }

    // add more entries
    {
      JsonDB db("test1", intkeys); // open DB
      db.secondary_open("names",  true);  // duplicated entries allowed
      db.secondary_open("unames", false); // duplicated entries not allowed

      ASSERT_FALSE(db.is_empty(), "db is empty!");
      Json j3 = Json::load_string(c3);
      j3.set("id","e4");
      db.put(j3);

      try{
        db.put(j3, false);
        ASSERT_TRUE(false, "overwriting");
      } catch(JsonDB::Err e){};

      try{
        j3.set("id",100);
        db.put(j3);
        ASSERT_TRUE(false, "int key");
      } catch(JsonDB::Err e){};

      Json j4 = Json::load_string(c4);
      Json j5 = Json::load_string(c5);
      j4.set("id","e5");
      j5.set("id","e6");
      db.put(j4);
      db.put(j5);

      try{
        j5.set("id",-1);
        db.put(j5);
        ASSERT_TRUE(false, "not an integer key");
      } catch(JsonDB::Err e){};

      try{
        Json j = Json::load_string("{\"names\": \"1\", \"unames\": 2, \"id\":\"z1\"}");
        db.put(j);
        ASSERT_TRUE(false, "string or array expected for unames");
      } catch(JsonDB::Err e){};

      try{
        Json j = Json::load_string("{\"names\": \"d\", \"unames\": \"a1\", \"id\":\"z2\"}");
        db.put(j);
        ASSERT_TRUE(false, "non-unique key");
      } catch(JsonDB::Err e){};

      try{
        db.get_last_id();
        ASSERT_TRUE(false, "not an integer key");
      } catch(JsonDB::Err e){};
    }

    const char *q1 = "{\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10, \"id\": \"e1\"}";
    const char *q2 = "{\"names\": [{\"id\": \"d\", \"value\": 0}, \"d\", \"c\"], \"unames\": \"a2\", \"value\": 10, \"id\": \"e2\"}";
    const char *q3 = "{\"names\": \"a\", \"unames\": {\"id\": \"a3\"}, \"value\": 30, \"id\": \"e4\"}";
    const char *q4 = "{\"unames\": [\"a4\"], \"value\": 40, \"id\": \"e5\"}";
    const char *q5 = "{\"names\": \"c\", \"unames\": [\"a5\"], \"value\": 50, \"id\": \"e6\"}";

    // check entries
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      ASSERT_FALSE(db.is_empty(), "db is empty!");
      ASSERT_TRUE(db.exists("e1"), "exist key1");
      ASSERT_TRUE(db.exists("e2"), "exist key2");
      ASSERT_FALSE(db.exists("e3"), "exist key3");
      ASSERT_TRUE(db.exists("e4"), "exist key4");


      ASSERT_EQ(db.get_str("e1"), q1, "get_str1");
      ASSERT_EQ(db.get_str("e2"), q2, "get_str2");
      ASSERT_EQ(db.get_str("e4"), q3, "get_str3");
      ASSERT_EQ(db.get_str("e5"), q4, "get_str4");
      ASSERT_EQ(db.get_str("e6"), q5, "get_str5");
      ASSERT_NE(db.get_str("e2"), q3, "get_str3");

      std::string res0 = (db.get_all()).save_string(JSON_PRESERVE_ORDER);
      std::string exp0;
      exp0 = exp0 +  "[" + q1
                  + ", " + q2
                  + ", " + q3
                  + ", " + q4
                  + ", " + q5 + "]";
      ASSERT_EQ(res0, exp0, "get_all");

    }

    // check secondary dbs
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      db.secondary_open("names",  true);  // duplicated entries allowed
      db.secondary_open("unames", false); // duplicated entries not allowed

      std::string resa  = (db.secondary_get("names", "a")).save_string(JSON_PRESERVE_ORDER);
      std::string resb  = (db.secondary_get("names", "b")).save_string(JSON_PRESERVE_ORDER);
      std::string resc  = (db.secondary_get("names", "c")).save_string(JSON_PRESERVE_ORDER);
      std::string resd  = (db.secondary_get("names", "d")).save_string(JSON_PRESERVE_ORDER);
      std::string resa2 = (db.secondary_get("unames", "a2")).save_string(JSON_PRESERVE_ORDER);

      std::string expa,expb,expc,expd;
      expa = expa + "[" + q1 + ", " + q3 + "]";
      expb = expb + "[" + q1 + "]";
      expc = expc + "[" + q1 + ", " + q2 + ", " + q5 + "]";
      expd = expd + "[" + q2 + "]";

      ASSERT_EQ(resa, expa, "find names == a");
      ASSERT_EQ(resb, expb, "find names == b");
      ASSERT_EQ(resc, expc, "find names == c");
      ASSERT_EQ(resd, expd, "find names == d");
      ASSERT_EQ(resa2,expd, "find unames == a2");

      ASSERT_TRUE(db.secondary_exists("names",   "a"), "secondary_exists - a");
      ASSERT_TRUE(db.secondary_exists("unames", "a2"), "secondary_exists - a2");
      ASSERT_FALSE(db.secondary_exists("names",   "z"), "secondary_exists - z");
      ASSERT_FALSE(db.secondary_exists("unames", "z2"), "secondary_exists - z2");

    }

  }
  catch (JsonDB::Err e){
    std::cerr << e.json() << "\n";
  }
  catch (Json::Err e){
    std::cerr << e.json() << "\n";
  }
  return 0;
}
