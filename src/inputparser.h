#ifndef H_INPUTPARSER
#define H_INPUTPARSER


#include "gamestatemanager.h"

class InputParser
{
public:
  InputParser(GameStateManager & gsm);

  /// Processing incoming packets.
  void immediateDispatch(int32_t eid, const std::vector<unsigned char> & data);

  /// Return true if a whole packet was extracted.
  bool dispatchIfEnoughData(int32_t eid, std::shared_ptr<SyncQueue> queue);

private:
  GameStateManager & m_gsm;
};


#endif
