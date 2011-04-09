/*
   Copyright (c) 2011, The Mineserver Project
   All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the The Mineserver Project nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "packets.h"

// ATTENTION: size EXCLUDES the initial type byte!

std::map<EPacketNames, PacketInfo> PACKET_INFO = {
  { PACKET_KEEP_ALIVE,               PacketInfo(0,                   "keep-alive") },
  { PACKET_LOGIN_REQUEST,            PacketInfo(PACKET_VARIABLE_LEN, "login request") },
  { PACKET_HANDSHAKE,                PacketInfo(PACKET_VARIABLE_LEN, "handshake") },
  { PACKET_CHAT_MESSAGE,             PacketInfo(PACKET_VARIABLE_LEN, "chat message") },
  { PACKET_USE_ENTITY,               PacketInfo(9,                   "use entity") },
  { PACKET_PLAYER,                   PacketInfo(1,                   "player") },
  { PACKET_PLAYER_POSITION,          PacketInfo(33,                  "player position") },
  { PACKET_PLAYER_LOOK,              PacketInfo(9,                   "player look") },
  { PACKET_PLAYER_POSITION_AND_LOOK, PacketInfo(41,                  "player pos+look") },
  { PACKET_PLAYER_DIGGING,           PacketInfo(11,                  "player digging") },
  { PACKET_PLAYER_BLOCK_PLACEMENT,   PacketInfo(PACKET_VARIABLE_LEN, "player block placement") },
  { PACKET_HOLDING_CHANGE,           PacketInfo(2,                   "player holding change") },
  { PACKET_ARM_ANIMATION,            PacketInfo(5,                   "arm animation") },
  { PACKET_PICKUP_SPAWN,             PacketInfo(22,                  "pickup spawn") },
  { PACKET_DISCONNECT,               PacketInfo(PACKET_VARIABLE_LEN, "disconnect") },
  { PACKET_RESPAWN,                  PacketInfo(0,                   "respawn") },
  { PACKET_INVENTORY_CHANGE,         PacketInfo(PACKET_VARIABLE_LEN, "inventory change") },
  { PACKET_INVENTORY_CLOSE,          PacketInfo(1,                   "inventory close") },
  { PACKET_SIGN,                     PacketInfo(PACKET_VARIABLE_LEN, "sign") },
  { PACKET_TRANSACTION,              PacketInfo(4,                   "transaction") },
  { PACKET_ENTITY_CROUCH,            PacketInfo(5,                   "entity crouch") }
};
