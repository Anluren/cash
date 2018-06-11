#include "common.h"

TEST_CASE("misc", "[misc]") {
  SECTION("utils", "[utils]") {
    TESTX([]()->bool {
      char bigs[258];
      memset(bigs, ' ', sizeof(bigs));
      bigs[255] = '%';
      bigs[256] = 'd';
      bigs[257] = '\0';
      std::string s = stringf(bigs, 7);
      std::cout << s << " " << s.size() << std::endl;
      return (s.size() == 256 && s[255] == '7');
    });
  }
  
  SECTION("assert", "[assert]") {
    TEST([]()->ch_bool {
      ch_bit4 a(1100_b);
      auto c = a.slice<2>(1) ^ 01_b;
      ch_assert(c == 11_b, "assertion failed!");
      return (c == 11_b);
    });
    TEST([]()->ch_bool {
      ch_bit4 a(1100_b), b(1);
      auto c = a.slice<2>(1) ^ 01_b;
      //ch_print("c={0}", c);
      __if (b == 1) {
        ch_assert(c == 11_b, "assertion failed!");
      } __else {
        ch_assert(c != 11_b, "assertion failed!");
      };
      return (c == 11_b);
    });
  }
  
  SECTION("taps", "[tap]") {
    TEST([]()->ch_bool {
      ch_bit4 a(1100_b);
      auto c = a.slice<2>(1) ^ 01_b;
      __tap(c);
      return (c == 11_b);
    });
  }
  
  SECTION("tick", "[tick]") {
    TEST([]()->ch_bool {
      ch_print("tick={0}", ch_time());
      return ch_true;
    });
  }
  
  SECTION("print", "[print]") {
    TEST([]()->ch_bool {
      ch_print("hello world");
      return ch_true;
    });
    TEST([]()->ch_bool {
      ch_bit8 a(255);
      ch_print("a={0}", a);
      return ch_true;
    });    
    TEST([]()->ch_bool {
      ch_bit8 a(255), b(0);
      ch_print("a={0}, b={1}", a, b);
      return ch_true;
    });  
    TEST([]()->ch_bool {
      ch_bit8 a(255);
      ch_bool b(1);
      __if (b) {
        ch_print("a={0}", a);
      };
      return ch_true;
    });
  }
}