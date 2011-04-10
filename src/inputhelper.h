#ifndef H_INPUTHELPER
#define H_INPUTHELPER


#include <list>
#include <deque>
#include <vector>
#include <string>


/* The #defines are for reading from a std::vector,
 *  the read*() functions are for reversible reading from the deque.
 */

#define READ_INT8(data, i)   ((unsigned int)(data[i]))
#define READ_INT16(data, i)  (int)((unsigned int)(data[i+1]) | ((unsigned int)(data[i]) << 8))
#define READ_INT32(data, i)  (int)((unsigned int)(data[i+3]) | (unsigned int)(data[i+2]) << 8 | (unsigned int)(data[i+1]) << 16 | ((unsigned int)(data[i]) << 24))
#define READ_BOOL(data, i)   (data[i] != 0)
#define READ_ANGLEFROMBYTE(data, i)   (double(data[i]) / 256. * 360.)

static inline double READ_DOUBLE(const std::vector<unsigned char> & data, size_t i)
{
  double y;
  char * c = reinterpret_cast<char*>(&y);
  c[7] = data[i]; c[6] = data[i+1]; c[5] = data[i+2]; c[4] = data[i+3]; c[3] = data[i+4]; c[2] = data[i+5]; c[1] = data[i+6]; c[0] = data[i+7];
  return y;
}

static inline float READ_FLOAT(const std::vector<unsigned char> & data, size_t i)
{
  float y;
  char * c = reinterpret_cast<char*>(&y);
  c[3] = data[i]; c[2] = data[i+1]; c[1] = data[i+2]; c[0] = data[i+3];
  return y;
}

static inline int8_t readInt8(std::deque<unsigned char> & q, std::list<unsigned char> journal)
{
  char c = q.front(); journal.push_back(c); q.pop_front();
  return c;
}

static inline int16_t readInt16(std::deque<unsigned char> & q, std::list<unsigned char> journal)
{
  char c;
  uint16_t r = 0;
  c = q.front(); journal.push_back(c); r |= ((uint16_t)(unsigned char)(c) <<  8); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint16_t)(unsigned char)(c) <<  0); q.pop_front();
  return int16_t(r);
}

static inline int32_t readInt32(std::deque<unsigned char> & q, std::list<unsigned char> journal)
{
  char c;
  uint32_t r = 0;
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) << 24); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) << 16); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) <<  8); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint32_t)(unsigned char)(c) <<  0); q.pop_front();
  return int32_t(r);
}

static inline int64_t readInt64(std::deque<unsigned char> & q, std::list<unsigned char> journal)
{
  char c;
  uint64_t r = 0;
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 56); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 48); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 40); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 32); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 24); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) << 16); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) <<  8); q.pop_front();
  c = q.front(); journal.push_back(c); r |= ((uint64_t)(unsigned char)(c) <<  0); q.pop_front();
  return int64_t(r);
}

static inline std::string readString(std::deque<unsigned char> & q, std::list<unsigned char> journal, uint16_t len)
{
  std::string r(len, 0);
  for (size_t i = 0; i < len; ++i)
  {
    r[i] = q.front();
    journal.push_back(q.front());
    q.pop_front();
  }
  return r;
}

static inline void rewindJournal(std::deque<unsigned char> & q, std::list<unsigned char> journal)
{
  q.insert(q.begin(), journal.begin(), journal.end());
}


#endif
