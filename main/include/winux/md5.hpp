/* MD5
 converted to C++ class by Frank Thilo (thilo@unix-ag.org)
 for bzflag (http://www.bzflag.org)

   based on:

   md5.h and md5.c
   reference implementation of RFC 1321

   Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

*/

#ifndef BZF_MD5_H
#define BZF_MD5_H

namespace winux
{
// a small class for calculating MD5 hashes of strings or byte arrays
// it is not meant to be fast or secure
//
// usage: 1) feed it blocks of uchars with update()
//      2) finalize()
//      3) get hexdigest() string
//      or
//      MD5(AnsiString).hexdigest()
//
// assumes that char is 8 bit and int is 32 bit

class WINUX_DLL MD5
{
public:
  typedef unsigned int size_type; // must be 32bit
  typedef unsigned char uint8; //  8bit
  typedef unsigned int uint32;  // 32bit

  MD5();
  MD5(const AnsiString& content);
  void update(const unsigned char buf[], size_type length);
  void update(const char buf[], size_type length);
  MD5& finalize();

  AnsiString hexdigest() const;
  Buffer digestres() const;
  AnsiString toString() const; //!< hexdigest()
  friend std::ostream& operator<<(std::ostream& out, MD5& md5);

private:
  void init();
  enum {blocksize = 64}; //!< VC6 won't eat a const static int here

  void transform(const uint8 block[blocksize]);
  static void Decode(uint32 output[], const uint8 input[], size_type len);
  static void Encode(uint8 output[], const uint32 input[], size_type len);

  bool finalized;
  uint8 buffer[blocksize]; //!< bytes that didn't fit in last 64 byte chunk
  uint32 count[2];   //!< 64bit counter for number of bits (lo, hi)
  uint32 state[4];   //!< digest so far
  uint8 digest[16]; //!< the result

  // low level logic operations
  static inline uint32 F(uint32 x, uint32 y, uint32 z);
  static inline uint32 G(uint32 x, uint32 y, uint32 z);
  static inline uint32 H(uint32 x, uint32 y, uint32 z);
  static inline uint32 I(uint32 x, uint32 y, uint32 z);
  static inline uint32 RotateLeft(uint32 x, int n);
  static inline void FF(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac);
  static inline void GG(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac);
  static inline void HH(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac);
  static inline void II(uint32 &a, uint32 b, uint32 c, uint32 d, uint32 x, uint32 s, uint32 ac);
};

} // namespace winux

#endif
