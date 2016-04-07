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
      db.open_sec("names",  true);  // duplicated entries allowed
      db.open_sec("unames", false); // duplicated entries not allowed

      ASSERT_TRUE(db.is_empty(), "db is not empty!");
      db.put_json(1, Json::load_string(c1)); // put json into db
      db.put_str(2, c2); // put string directly
    }
    // add more entries
    {
      JsonDB db("test1", intkeys); // open DB
      db.open_sec("names",  true);  // duplicated entries allowed
      db.open_sec("unames", false); // duplicated entries not allowed

      ASSERT_FALSE(db.is_empty(), "db is empty!");
      db.put_str(4, c3);

      try{
        db.put_str("10", c1, false);
        ASSERT_TRUE(false, "string key");
      } catch(JsonDB::Err e){};

      try{
        db.put_str(4, c1, false);
        ASSERT_TRUE(false, "overwriting");
      } catch(JsonDB::Err e){};

      db.put_str_next(c4);
      db.put_json_next(Json::load_string(c5));

      try{
        db.put_str_next("{\"names\": \"1\", \"unames\": 2}");
        ASSERT_TRUE(false, "string or array expected for unames");
      } catch(JsonDB::Err e){};

      try{
        db.put_str_next("{\"names\": \"d\", \"unames\": \"a1\"}");
        ASSERT_TRUE(false, "non-unique key");
      } catch(JsonDB::Err e){};
    }

    // check entries
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      ASSERT_FALSE(db.is_empty(), "db is empty!");
      ASSERT_TRUE(db.exists(1), "exist key1");
      ASSERT_TRUE(db.exists(2), "exist key2");
      ASSERT_FALSE(db.exists(3), "exist key3");
      ASSERT_TRUE(db.exists(4), "exist key4");
      ASSERT_EQ(db.get_str(1), c1, "get_str1");
      ASSERT_EQ(db.get_str(2), c2, "get_str2");
      ASSERT_EQ(db.get_str(4), c3, "get_str3");
      ASSERT_EQ(db.get_str(5), c4, "get_str4");
      ASSERT_EQ(db.get_str(6), c5, "get_str5");
      ASSERT_NE(db.get_str(2), c3, "get_str3");
      ASSERT_EQ(db.get_last_id(), 6, "last_id");

      std::string res0 = db.get_all().save_string(JSON_PRESERVE_ORDER);
      std::string exp0;
      exp0 = exp0 +  "{\"1\": " + c1
                  + ", \"2\": " + c2
                  + ", \"4\": " + c3
                  + ", \"5\": " + c4
                  + ", \"6\": " + c5 + "}";
      ASSERT_EQ(res0, exp0, "get_all");

    }

    // check secondary dbs
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      db.open_sec("names",  true);  // duplicated entries allowed
      db.open_sec("unames", false); // duplicated entries not allowed

      std::string resa  = (db.secondary_get("names", "a")).save_string(JSON_PRESERVE_ORDER);
      std::string resb  = (db.secondary_get("names", "b")).save_string(JSON_PRESERVE_ORDER);
      std::string resc  = (db.secondary_get("names", "c")).save_string(JSON_PRESERVE_ORDER);
      std::string resd  = (db.secondary_get("names", "d")).save_string(JSON_PRESERVE_ORDER);
      std::string resa2 = (db.secondary_get("unames", "a2")).save_string(JSON_PRESERVE_ORDER);

      const char *expa = "{\"1\": {\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10}, \"4\": {\"names\": \"a\", \"unames\": {\"id\": \"a3\"}, \"value\": 30}}";
      const char *expb = "{\"1\": {\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10}}";
      const char *expc = "{\"1\": {\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10}, \"2\": {\"names\": [{\"id\": \"d\", \"value\": 0}, \"d\", \"c\"], \"unames\": \"a2\", \"value\": 10}, \"6\": {\"names\": \"c\", \"unames\": [\"a5\"], \"value\": 50}}";
      const char *expd = "{\"2\": {\"names\": [{\"id\": \"d\", \"value\": 0}, \"d\", \"c\"], \"unames\": \"a2\", \"value\": 10}}";

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
      db.open_sec("names",  true);  // duplicated entries allowed
      db.open_sec("unames", false); // duplicated entries not allowed
      const char * c1 = "{\"a\": \"string1\"}";
      size_t t1 = db.put_str_time(c1);
      size_t t2 = db.put_str_time(c1);
      ASSERT_NE(t1,t2,"log1");
      ASSERT_EQ(db.get_str(t1),c1,"log2");
      ASSERT_EQ(db.get_str(t2),c1,"log3");
    }

    /*********************************************************************************/
    // repeat the same with string keys
    intkeys = false;
    {
      JsonDB db("test1", intkeys, DB_TRUNCATE | DB_CREATE); // open DB
      db.open_sec("names",  true);  // duplicated entries allowed
      db.open_sec("unames", false); // duplicated entries not allowed

      ASSERT_TRUE(db.is_empty(), "db is not empty!");
      db.put_json("e1", Json::load_string(c1)); // put json into db
      db.put_str("e2", c2); // put string directly
    }

    // add more entries
    {
      JsonDB db("test1", intkeys); // open DB
      db.open_sec("names",  true);  // duplicated entries allowed
      db.open_sec("unames", false); // duplicated entries not allowed

      ASSERT_FALSE(db.is_empty(), "db is empty!");
      db.put_str("e4", c3);

      try{
        db.put_str("e4", c1, false);
        ASSERT_TRUE(false, "overwriting");
      } catch(JsonDB::Err e){};

      try{
        db.put_str(4, c1, false);
        ASSERT_TRUE(false, "int key");
      } catch(JsonDB::Err e){};

      db.put_str("e5", c4);
      db.put_json("e6", Json::load_string(c5));

      try{
        db.put_str_next("{\"names\": \"1\", \"unames\": \"2\"}");
        ASSERT_TRUE(false, "not an integer key");
      } catch(JsonDB::Err e){};

      try{
        db.put_str_next("{\"names\": \"1\", \"unames\": 2}");
        ASSERT_TRUE(false, "string or array expected for unames");
      } catch(JsonDB::Err e){};

      try{
        db.put_str_next("{\"names\": \"d\", \"unames\": \"a1\"}");
        ASSERT_TRUE(false, "non-unique key");
      } catch(JsonDB::Err e){};

      try{
        db.get_last_id();
        ASSERT_TRUE(false, "not an integer key");
      } catch(JsonDB::Err e){};
    }


    // check entries
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      ASSERT_FALSE(db.is_empty(), "db is empty!");
      ASSERT_TRUE(db.exists("e1"), "exist key1");
      ASSERT_TRUE(db.exists("e2"), "exist key2");
      ASSERT_FALSE(db.exists("e3"), "exist key3");
      ASSERT_TRUE(db.exists("e4"), "exist key4");
      ASSERT_EQ(db.get_str("e1"), c1, "get_str1");
      ASSERT_EQ(db.get_str("e2"), c2, "get_str2");
      ASSERT_EQ(db.get_str("e4"), c3, "get_str3");
      ASSERT_EQ(db.get_str("e5"), c4, "get_str4");
      ASSERT_EQ(db.get_str("e6"), c5, "get_str5");
      ASSERT_NE(db.get_str("e2"), c3, "get_str3");

      std::string res0 = (db.get_all()).save_string(JSON_PRESERVE_ORDER);
      std::string exp0;
      exp0 = exp0 +  "{\"e1\": " + c1
                  + ", \"e2\": " + c2
                  + ", \"e4\": " + c3
                  + ", \"e5\": " + c4
                  + ", \"e6\": " + c5 + "}";
      ASSERT_EQ(res0, exp0, "get_all");

    }

    // check secondary dbs
    {
      JsonDB db("test1", intkeys, DB_RDONLY); // open DB
      db.open_sec("names",  true);  // duplicated entries allowed
      db.open_sec("unames", false); // duplicated entries not allowed

      std::string resa  = (db.secondary_get("names", "a")).save_string(JSON_PRESERVE_ORDER);
      std::string resb  = (db.secondary_get("names", "b")).save_string(JSON_PRESERVE_ORDER);
      std::string resc  = (db.secondary_get("names", "c")).save_string(JSON_PRESERVE_ORDER);
      std::string resd  = (db.secondary_get("names", "d")).save_string(JSON_PRESERVE_ORDER);
      std::string resa2 = (db.secondary_get("unames", "a2")).save_string(JSON_PRESERVE_ORDER);

      const char *expa = "{\"e1\": {\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10}, \"e4\": {\"names\": \"a\", \"unames\": {\"id\": \"a3\"}, \"value\": 30}}";
      const char *expb = "{\"e1\": {\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10}}";
      const char *expc = "{\"e1\": {\"names\": [\"a\", \"b\", \"c\"], \"unames\": [\"a1\", \"b1\"], \"value\": 10}, \"e2\": {\"names\": [{\"id\": \"d\", \"value\": 0}, \"d\", \"c\"], \"unames\": \"a2\", \"value\": 10}, \"e6\": {\"names\": \"c\", \"unames\": [\"a5\"], \"value\": 50}}";
      const char *expd = "{\"e2\": {\"names\": [{\"id\": \"d\", \"value\": 0}, \"d\", \"c\"], \"unames\": \"a2\", \"value\": 10}}";

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
