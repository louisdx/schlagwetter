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

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <array>

#define PACKET_NEED_MORE_DATA -3
#define PACKET_DOES_NOT_EXIST -2
#define PACKET_VARIABLE_LEN   -1
#define PACKET_OK             0

/************ Packet Names *************/

enum EPacketNames
{
  //Client to server (not exclusively)
  PACKET_KEEP_ALIVE                = 0x00,
  PACKET_LOGIN_REQUEST             = 0x01,
  PACKET_HANDSHAKE                 = 0x02,
  PACKET_CHAT_MESSAGE              = 0x03,
  PACKET_ENTITY_EQUIPMENT          = 0x05,
  PACKET_RESPAWN                   = 0x09,
  PACKET_PLAYER                    = 0x0a,
  PACKET_PLAYER_POSITION           = 0x0b,
  PACKET_PLAYER_LOOK               = 0x0c,
  PACKET_PLAYER_POSITION_AND_LOOK  = 0x0d,
  PACKET_PLAYER_DIGGING            = 0x0e,
  PACKET_PLAYER_BLOCK_PLACEMENT    = 0x0f,
  PACKET_HOLDING_CHANGE            = 0x10,
  PACKET_ARM_ANIMATION             = 0x12,
  PACKET_ENTITY_CROUCH             = 0x13,
  PACKET_INVENTORY_CLOSE           = 0x65,
  PACKET_INVENTORY_CHANGE          = 0x66,
  PACKET_SET_SLOT                  = 0x67,
  PACKET_INVENTORY                 = 0x68,
  PACKET_SIGN                      = 0x82,
  PACKET_DISCONNECT                = 0xff,

  //Server to client (not exclusively)
  PACKET_LOGIN_RESPONSE            = 0x01,
  PACKET_TIME_UPDATE               = 0x04,
  PACKET_SPAWN_POSITION            = 0x06,
  PACKET_UPDATE_HEALTH             = 0x08,
  PACKET_ADD_TO_INVENTORY          = 0x11,
  PACKET_NAMED_ENTITY_SPAWN        = 0x14,
  PACKET_PICKUP_SPAWN              = 0x15,
  PACKET_COLLECT_ITEM              = 0x16,
  PACKET_ADD_OBJECT                = 0x17,
  PACKET_MOB_SPAWN                 = 0x18,
  PACKET_DESTROY_ENTITY            = 0x1d,
  PACKET_ENTITY                    = 0x1e,
  PACKET_ENTITY_RELATIVE_MOVE      = 0x1f,
  PACKET_ENTITY_LOOK               = 0x20,
  PACKET_ENTITY_LOOK_RELATIVE_MOVE = 0x21,
  PACKET_ENTITY_TELEPORT           = 0x22,
  PACKET_DEATH_ANIMATION           = 0x26,
  PACKET_PRE_CHUNK                 = 0x32,
  PACKET_MAP_CHUNK                 = 0x33,
  PACKET_MULTI_BLOCK_CHANGE        = 0x34,
  PACKET_BLOCK_CHANGE              = 0x35,
  PACKET_PLAY_NOTE                 = 0x36,
  PACKET_OPEN_WINDOW               = 0x64,
  PACKET_PROGRESS_BAR              = 0x69,
  PACKET_TRANSACTION               = 0x6a,
  // PACKET_COMPLEX_ENTITIES       = 0x3b,
  PACKET_KICK                      = 0xff,

  //v4 Packets
  PACKET_USE_ENTITY                = 0x07,
  PACKET_ENTITY_VELOCITY           = 0x1c,
  PACKET_ATTACH_ENTITY             = 0x27
};


/*********** Packet Info Types ***********/

struct PacketInfo
{
  size_t size;
  std::string name;
  explicit PacketInfo(size_t size = 0, std::string name = "") : size(size), name(name) { }
};

/// enums don't hash by default, but since they're integral types, we can just use the int hasher.
namespace std {
  template<>
  struct hash<EPacketNames> : public unary_function<EPacketNames, size_t>
  {
    static hash<int> hasher;
    inline size_t operator()(const EPacketNames & v) const
    {
      return hasher(int(v));
    }
  };
}

extern std::unordered_map<EPacketNames, PacketInfo> PACKET_INFO;


/******** Packet Data Structures *********/

struct packet_login_request
{
  int32_t     version;
  std::string Username;
  std::string Password;
  int64_t     map_seed;
  int8_t      dimension;
};

struct packet_player_position
{
  double      x;
  double      y;
  double      stance;
  double      z;
  uint8_t     onground;
};

struct packet_player_look
{
  float       yaw;
  float       pitch;
  uint8_t     onground;
};

struct packet_player_position_and_look
{
  double     x;
  double     y;
  double     stance;
  double     z;
  float      yaw;
  float      pitch;
  uint8_t    onground;
};


/************ Constants *************/

// Light emission/absorption

typedef std::unordered_map<unsigned char, unsigned char> LightMapMap;

struct LightMap
{
public:

  LightMap(unsigned char defval, std::initializer_list<LightMapMap::value_type> lm)
  :  m_defval(defval), m_light_map(lm), m_it(), m_end(m_light_map.end())
  {  }

  inline unsigned char operator[](unsigned char block)
  {
    m_it = m_light_map.find(block);
    return m_it == m_end ? m_defval : m_it->second;
  }
private:
  const unsigned char            m_defval;
  const LightMapMap           m_light_map;
        LightMapMap::const_iterator  m_it;
  const LightMapMap::const_iterator m_end;
};

extern LightMap EMIT_LIGHT;
extern LightMap STOP_LIGHT;

//Player digging status
enum
{
  DIGGING_STARTED = 0,
  DIGGING_FINISHED = 2,
  DIGGING_DROPITEM = 4
};

// Chat colors
#define MC_COLOR_BLACK std::string("§0")
#define MC_COLOR_DARK_BLUE std::string("§1")
#define MC_COLOR_DARK_GREEN std::string("§2")
#define MC_COLOR_DARK_CYAN std::string("§3")
#define MC_COLOR_DARK_RED std::string("§4")
#define MC_COLOR_DARK_MAGENTA std::string("§5")
#define MC_COLOR_DARK_ORANGE std::string("§6")
#define MC_COLOR_GREY std::string("§7")
#define MC_COLOR_DARK_GREY std::string("§8")
#define MC_COLOR_BLUE std::string("§9")
#define MC_COLOR_GREEN std::string("§a")
#define MC_COLOR_CYAN std::string("§b")
#define MC_COLOR_RED std::string("§c")
#define MC_COLOR_MAGENTA std::string("§d")
#define MC_COLOR_YELLOW std::string("§e")
#define MC_COLOR_WHITE std::string("§f")

// Blocks
enum Block
{
  BLOCK_AIR, BLOCK_STONE, BLOCK_GRASS, BLOCK_DIRT, BLOCK_COBBLESTONE, BLOCK_PLANK,
  BLOCK_SAPLING, BLOCK_BEDROCK, BLOCK_WATER, BLOCK_STATIONARY_WATER, BLOCK_LAVA,
  BLOCK_STATIONARY_LAVA, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_GOLD_ORE, BLOCK_IRON_ORE,
  BLOCK_COAL_ORE, BLOCK_WOOD, BLOCK_LEAVES, BLOCK_SPONGE, BLOCK_GLASS, BLOCK_LAPIS_ORE,
  BLOCK_LAPIS_BLOCK, BLOCK_DISPENSER, BLOCK_SANDSTONE, BLOCK_NOTE_BLOCK, BLOCK_BED,
  BLOCK_YELLOW_FLOWER = 37, BLOCK_RED_ROSE, BLOCK_BROWN_MUSHROOM, BLOCK_RED_MUSHROOM,
  BLOCK_GOLD_BLOCK, BLOCK_IRON_BLOCK, BLOCK_DOUBLE_STEP, BLOCK_STEP, BLOCK_BRICK,
  BLOCK_TNT, BLOCK_BOOKSHELF, BLOCK_MOSSY_COBBLESTONE, BLOCK_OBSIDIAN, BLOCK_TORCH,
  BLOCK_FIRE, BLOCK_MOB_SPAWNER, BLOCK_WOODEN_STAIRS, BLOCK_CHEST, BLOCK_REDSTONE_WIRE,
  BLOCK_DIAMOND_ORE, BLOCK_DIAMOND_BLOCK, BLOCK_WORKBENCH, BLOCK_CROPS, BLOCK_SOIL,
  BLOCK_FURNACE, BLOCK_BURNING_FURNACE, BLOCK_SIGN_POST, BLOCK_WOODEN_DOOR,
  BLOCK_LADDER, BLOCK_MINECART_TRACKS, BLOCK_COBBLESTONE_STAIRS, BLOCK_WALL_SIGN,
  BLOCK_LEVER, BLOCK_STONE_PRESSURE_PLATE, BLOCK_IRON_DOOR, BLOCK_WOODEN_PRESSURE_PLATE,
  BLOCK_REDSTONE_ORE, BLOCK_GLOWING_REDSTONE_ORE, BLOCK_REDSTONE_TORCH_OFF,
  BLOCK_REDSTONE_TORCH_ON, BLOCK_STONE_BUTTON, BLOCK_SNOW, BLOCK_ICE, BLOCK_SNOW_BLOCK,
  BLOCK_CACTUS, BLOCK_CLAY, BLOCK_REED, BLOCK_JUKEBOX, BLOCK_FENCE, BLOCK_PUMPKIN,
  BLOCK_NETHERSTONE, BLOCK_SLOW_SAND, BLOCK_GLOWSTONE, BLOCK_PORTAL, BLOCK_JACK_O_LANTERN,
  BLOCK_CAKE, BLOCK_REDSTONE_REPEATER_OFF, BLOCK_REDSTONE_REPEATER_ON, BLOCK_LOCKED_CHEST,
  BLOCK_AQUA_GREEN_CLOTH, BLOCK_CYAN_CLOTH, BLOCK_BLUE_CLOTH, BLOCK_PURPLE_CLOTH,
  BLOCK_INDIGO_CLOTH, BLOCK_VIOLET_CLOTH, BLOCK_MAGENTA_CLOTH, BLOCK_PINK_CLOTH,
  BLOCK_BLACK_CLOTH, BLOCK_WHITE_CLOTH, BLOCK_GRAY_CLOTH = 35
};

// Items
enum
{
  ITEM_IRON_SPADE = 256, ITEM_IRON_PICKAXE, ITEM_IRON_AXE, ITEM_FLINT_AND_STEEL, ITEM_APPLE,
  ITEM_BOW, ITEM_ARROW, ITEM_COAL, ITEM_DIAMOND, ITEM_IRON_INGOT, ITEM_GOLD_INGOT, ITEM_IRON_SWORD,
  ITEM_WOODEN_SWORD, ITEM_WOODEN_SPADE, ITEM_WOODEN_PICKAXE, ITEM_WOODEN_AXE, ITEM_STONE_SWORD,
  ITEM_STONE_SPADE, ITEM_STONE_PICKAXE, ITEM_STONE_AXE, ITEM_DIAMOND_SWORD,
  ITEM_DIAMOND_SPADE, ITEM_DIAMOND_PICKAXE, ITEM_DIAMOND_AXE, ITEM_STICK, ITEM_BOWL,
  ITEM_MUSHROOM_SOUP, ITEM_GOLD_SWORD, ITEM_GOLD_SPADE, ITEM_GOLD_PICKAXE, ITEM_GOLD_AXE,
  ITEM_STRING, ITEM_FEATHER, ITEM_GUNPOWDER, ITEM_WOODEN_HOE, ITEM_STONE_HOE,
  ITEM_IRON_HOE, ITEM_DIAMOND_HOE, ITEM_GOLD_HOE, ITEM_SEEDS, ITEM_WHEAT, ITEM_BREAD,
  ITEM_LEATHER_HELMET, ITEM_LEATHER_CHESTPLATE, ITEM_LEATHER_LEGGINGS, ITEM_LEATHER_BOOTS,
  ITEM_CHAINMAIL_HELMET, ITEM_CHAINMAIL_CHESTPLATE, ITEM_CHAINMAIL_LEGGINGS,
  ITEM_CHAINMAIL_BOOTS, ITEM_IRON_HELMET, ITEM_IRON_CHESTPLATE, ITEM_IRON_LEGGINGS,
  ITEM_IRON_BOOTS, ITEM_DIAMOND_HELMET, ITEM_DIAMOND_CHESTPLATE, ITEM_DIAMOND_LEGGINGS,
  ITEM_DIAMOND_BOOTS, ITEM_GOLD_HELMET, ITEM_GOLD_CHESTPLATE, ITEM_GOLD_LEGGINGS,
  ITEM_GOLD_BOOTS, ITEM_FLINT, ITEM_PORK, ITEM_GRILLED_PORK, ITEM_PAINTINGS,
  ITEM_GOLDEN_APPLE, ITEM_SIGN, ITEM_WOODEN_DOOR, ITEM_BUCKET, ITEM_WATER_BUCKET,
  ITEM_LAVA_BUCKET, ITEM_MINECART, ITEM_SADDLE, ITEM_IRON_DOOR, ITEM_REDSTONE,
  ITEM_SNOWBALL, ITEM_BOAT, ITEM_LEATHER, ITEM_MILK_BUCKET, ITEM_CLAY_BRICK,
  ITEM_CLAY_BALLS, ITEM_REED, ITEM_PAPER, ITEM_BOOK, ITEM_SLIME_BALL,
  ITEM_STORAGE_MINECART, ITEM_POWERED_MINECART, ITEM_EGG, ITEM_COMPASS, ITEM_FISHING_ROD,
  ITEM_WATCH, ITEM_GLOWSTONE_DUST, ITEM_RAW_FISH, ITEM_COOKED_FISH, ITEM_DYE,
  ITEM_BONE, ITEM_SUGAR, ITEM_CAKE, ITEM_BED
};

// Records
enum
{
  ITEM_GOLD_RECORD = 2256, ITEM_GREEN_RECORD
};

// Mobs
enum
{
  MOB_CREEPER = 50, MOB_SKELETON, MOB_SPIDER, MOB_GIANT_ZOMBIE, MOB_ZOMBIE,
  MOB_SLIME, MOB_GHAST, MOB_ZOMBIE_PIGMAN
};

// Animals
enum
{
  MOB_PIG = 90, MOB_SHEEP, MOB_COW, MOB_CHICKEN, MOB_SQUID
};

//Instruments (based off http://www.minecraftwiki.net/wiki/Note_Block)

enum
{
  INSTRUMENT_BASS = 1, INSTRUMENT_SNARE, INSTRUMENT_STICK, INSTRUMENT_BASSDRUM, INSTRUMENT_HARP
};

//
// Drops from blocks
//
struct Drop
{
  uint16_t item_id;
  uint32_t probability;
  uint8_t count;
  Drop* alt_drop;

  Drop() : item_id(0), probability(0), count(0), alt_drop(NULL) {}
  Drop(uint16_t _item_id, uint32_t _probability, uint8_t _count, Drop* _alt_drop = NULL) : item_id(_item_id), probability(_probability), count(_count), alt_drop(_alt_drop) {}

  ~Drop()
  {
    if (alt_drop != NULL)
    {
      delete alt_drop;
    }
  }
};

// Chat prefixes
const char SERVERMSGPREFIX = '%';
const char CHATCMDPREFIX   = '/';
const char ADMINCHATPREFIX = '&';

const unsigned int SERVER_CONSOLE_UID = -1;

//allocate 1 MB for chunk files
const int ALLOCATE_NBTFILE = 1048576;


#endif
