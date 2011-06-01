/*
Copyright (c) 2009, Micael Hildenborg
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Micael Hildenborg nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Micael Hildenborg ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Micael Hildenborg BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SHA1_DEFINED
#define SHA1_DEFINED

#include <string>

namespace sha1
{
	/**
		@param src points to any kind of data to be hashed.
		@param bytelength the number of bytes to hash from the src pointer.
		@param hash should point to a buffer of at least 20 bytes of size for storing the sha1 result in.
	*/
	void calc(const void *src, const int bytelength, unsigned char *hash);

        std::string calcToString(const void *src, const size_t bytelength);

} // namespace sha1



/* Thanks to "Advanced STL Part 5" for this neat piece of code. */

template <typename T> std::string hexify(const T & v)
{
  static const char HEXCHARS[] = "0123456789ABCDEF";

  std::string res(2 * v.size(), 'x');
  std::string::iterator k = res.begin();

  for (typename T::const_iterator i = v.begin(); i != v.end(); ++i)
  {
    *k++ = HEXCHARS[(unsigned char)(*i) >> 4];
    *k++ = HEXCHARS[(unsigned char)(*i) & 0x0F];
  }

  return res;
}


#endif // SHA1_DEFINED
