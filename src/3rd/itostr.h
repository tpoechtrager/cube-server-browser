/* http://stackoverflow.com/a/4364057 */

namespace itostr {

extern unsigned int vals[10000];

#ifdef __INTEL_COMPILER

//#pragma warning push
#pragma warning disable 186

#elif defined(__GNUC__)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif

#endif

template <typename T> void itostr(std::string &s, T o) {
  unsigned int blocks[3], *b = blocks + 2;
  blocks[0] = o < 0 ? ~o + 1 : o;
  blocks[2] = blocks[0] % 10000;
  blocks[0] /= 10000;
  blocks[2] = itostr::vals[blocks[2]];

  if (blocks[0]) {
    blocks[1] = blocks[0] % 10000;
    blocks[0] /= 10000;
    blocks[1] = itostr::vals[blocks[1]];
    blocks[2] |= 0x30303030;
    b--;
  }

  if (blocks[0]) {
    blocks[0] = itostr::vals[blocks[0] % 10000];
    blocks[1] |= 0x30303030;
    b--;
  }

  char *f = reinterpret_cast<char *>(b);
  f += 3 - (*f >> 4);

  char *str = reinterpret_cast<char *>(blocks);
  if (o < 0) *--f = '-';
  s.append(f, (str + 12) - f);
}

#ifdef __INTEL_COMPILER

//#pragma warning pop

#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} // namespace itostr
