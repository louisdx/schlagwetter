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

#include "constants.h"

// ATTENTION: Packet size EXCLUDES the initial type byte!

std::unordered_set<PacketInfo, std::hash<size_t>> PACKET_INFO = {
  { PACKET_KEEP_ALIVE,               0,                   "keep-alive"},
  { PACKET_LOGIN_REQUEST,            PACKET_VARIABLE_LEN, "login request"},
  { PACKET_HANDSHAKE,                PACKET_VARIABLE_LEN, "handshake"},
  { PACKET_PRE_CHUNK,                9,                   "chunk request"},
  { PACKET_CHAT_MESSAGE,             PACKET_VARIABLE_LEN, "chat message"},
  { PACKET_USE_ENTITY,               9,                   "use entity"},
  { PACKET_PLAYER,                   1,                   "player"},
  { PACKET_PLAYER_POSITION,          33,                  "player position"},
  { PACKET_PLAYER_LOOK,              9,                   "player look"},
  { PACKET_PLAYER_POSITION_AND_LOOK, 41,                  "player pos+look"},
  { PACKET_PLAYER_DIGGING,           11,                  "player digging"},
  { PACKET_PLAYER_BLOCK_PLACEMENT,   PACKET_VARIABLE_LEN, "player block placement"},
  { PACKET_HOLDING_CHANGE,           2,                   "player holding change"},
  { PACKET_ARM_ANIMATION,            5,                   "arm animation"},
  { PACKET_PICKUP_SPAWN,             22,                  "pickup spawn"},
  { PACKET_DISCONNECT,               PACKET_VARIABLE_LEN, "disconnect"},
  { PACKET_RESPAWN,                  0,                   "respawn"},
  { PACKET_INVENTORY_CHANGE,         PACKET_VARIABLE_LEN, "inventory change"},
  { PACKET_INVENTORY_CLOSE,          1,                   "inventory close"},
  { PACKET_SIGN,                     PACKET_VARIABLE_LEN, "sign"},
  { PACKET_TRANSACTION,              4,                   "transaction"},
  { PACKET_ENTITY_CROUCH,            5,                   "entity crouch"}
};

std::unordered_set<BlockItemInfo, std::hash<size_t>> BLOCKITEM_INFO = {
  { BLOCK_Air, "Air" },
  { BLOCK_Torch, "Torch"},
  { ITEM_DiamondShovel, "Diamond Shovel"}
};

LightMap EMIT_LIGHT(0, {
  { BLOCK_Lava, 15 }, // Lava
  { BLOCK_StationaryLava, 15 }, // Stationary Lava
  { BLOCK_BrownMushroom,  1 }, // Brown mushroom
  { BLOCK_Torch, 14 }, // Torch
  { BLOCK_Fire, 15 }, // Fire
  { 0x3E, 14 }, // Lit furnace
  { 0x4A,  9 }, // Redstone ore (Glowing)
  { 0x4C,  7 }, // Redstone Torch (On)
  { 0x59, 15 }, // Lightstone
  { 0x5A, 11 }, // Portal
  { 0x5B, 15 }, // Jack-O-Lantern
    } );

LightMap STOP_LIGHT(16, {
  { BLOCK_Air, 0 },
  { BLOCK_Sapling, 0 }, // Sapling
  { BLOCK_Water, 3 }, // Water
  { BLOCK_StationaryWater, 3 }, // Stationary water
  { 0x12, 3 }, // Leaves
  { 0x14, 0 }, // Glass
  { 0x25, 0 }, // Yellow flower
  { 0x26, 0 }, // Red rose
  { 0x27, 0 }, // Brown mushroom
  { 0x28, 0 }, // Red mushroom
  { 0x32, 0 }, // Torch
  { 0x33, 0 }, // Fire
  { 0x34, 0 }, // Mob spawner
  { 0x35, 0 }, // Wooden stairs
  { 0x37, 0 }, // Redstone wire
  { 0x40, 0 }, // Wooden door
  { 0x41, 0 }, // Ladder
  { 0x42, 0 }, // Minecart track
  { 0x43, 0 }, // Cobblestone stairs
  { 0x47, 0 }, // Iron door
  { 0x4b, 0 }, // Redstone Torch (Off)
  { 0x4C, 0 }, // Redstone Torch (On)
  { 0x4e, 0 }, // Snow
  { 0x4f, 3 }, // Ice
  { 0x55, 0 }, // Fence
  { 0x5A, 0 }, // Portal
  { 0x5B, 0 }, // Jack-O-Lantern
  { BLOCK_SignPost, 0 }, // Sign post
  { BLOCK_WallSign, 0 }, // Wall sign
    } );
