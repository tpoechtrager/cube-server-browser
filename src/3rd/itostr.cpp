#include <string>
#include "itostr.h"

namespace itostr {

/* http://stackoverflow.com/a/4364057 */

unsigned int vals[10000];

struct initialize {
  initialize() {
    for (unsigned int i = 0; i < 10000; ++i) {
      unsigned int v = i;
      char *o = reinterpret_cast<char *>(vals + i);
      o[3] = v % 10 + '0';
      o[2] = (v % 100) / 10 + '0';
      o[1] = (v % 1000) / 100 + '0';
      o[0] = (v % 10000) / 1000;
      if (o[0]) o[0] |= 0x30;
      else if (o[1] != '0') o[0] |= 0x20;
      else if (o[2] != '0') o[0] |= 0x10;
      else o[0] |= 0x00;
    }
  }
};

namespace {
initialize init;
} // anonymous namespace

} // namespace itostr
