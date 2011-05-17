#ifndef H_SALT
#define H_SALT

#include "random.h"
#include "sha1.h"

inline std::string generateHash(const std::string & plainText, uint32_t salt = uniformUINT32())
{
  const std::string salt_str(reinterpret_cast<char *>(&salt), 4);
  return salt_str + sha1::calcToString((salt_str + plainText).data(), 4 + plainText.length());
}

inline uint32_t getSalt(const std::string & str)
{
  return str.length() < 4 ? -1 : *reinterpret_cast<const uint32_t *>(str.data());
}

#endif
