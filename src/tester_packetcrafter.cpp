#include <cstdio>
#include <algorithm>
#include <iterator>
#include "packetcrafter.h"
#include "inputhelper.h"

template<typename S>
void hexString(SyncQueue d)
{
  printf("Deque of size = %u. Data: ", d.size());
  for (auto i = d.begin(); i != d.end(); ++i)
  {
    if (i != d.begin()) printf(", ");
    printf("%02X", (unsigned char)*i);
  }
  printf("\n");
}


int main()
{

  std::list<unsigned char> tmp;

  signed short int a = -1, b = 12, c = -20000;
  unsigned short int x = 234, y = 1, z = -1;

  PacketCrafter p(0xFF);

  p.addInt16(a);
  p.addInt16(b);
  p.addInt16(c);
  p.addInt16(x);
  p.addInt16(y);
  p.addInt16(z);

  std::string s = p.craft();
  std::deque<unsigned char> D;
  std::copy(s.begin(), s.end(), std::back_inserter(D));

  printf("In:  %d %d %d %u %u %u\n", a,b,c,x,y,z);
  hexString(D);

  int8_t c0 = readInt8(D, tmp);
  signed short int c1 = readInt16(D, tmp);
  signed short int c2 = readInt16(D, tmp);
  signed short int c3 = readInt16(D, tmp);
  unsigned short int c11 = readInt16(D, tmp);
  unsigned short int c12 = readInt16(D, tmp);
  unsigned short int c13 = readInt16(D, tmp);

  printf("Out: %d %d %d %d %u %u %u : Final size = %u\n", c0,c1,c2,c3,c11,c12,c13, D.size());


  signed int a1 = -2;
  unsigned int a2 = 11;

  PacketCrafter q(0xFF);
  q.addInt32(a1);
  q.addInt32(a2);
  std::string v = q.craft();

  std::deque<unsigned char> F;
  std::copy(v.begin(), v.end(), std::back_inserter(F));

  printf("\nIn:  %d %u\n", a1, a2);
  hexString(F);

  int8_t b0 = readInt8(F, tmp);
  signed int b1 = readInt32(F, tmp);
  unsigned int b2 = readInt32(F, tmp); 
  printf("Out: %d %d %u : Final size = %u\n", b0, b1, b2, F.size());


  printf("\n");
  std::deque<unsigned char> G;
  G.push_back(0xFF);
  G.push_back(0xFF);
  G.push_back(0xFF);
  G.push_back(0xFF);

  hexString(G);

  int16_t d1 = readInt16(G, tmp);
  uint16_t d2 = readInt16(G, tmp);
  printf("%d %u : Final size = %u\n", d1, d2, G.size());
}
