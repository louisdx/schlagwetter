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
#include <unordered_set>
#include <array>


/// The distance for which chunks need to be available to the client.
/// An actual (2 r + 1)^2 around the player is sent, see ambientChunks().

enum { PLAYER_CHUNK_HORIZON = 5 }; // Set to 5 for production, 2 for valgrinding.



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


/************ Blocks and Items *************/

/// Taken from the "craftd" project, "minecraft.h".

enum EBlockItem
{
  ITEM_IronShovel          = 0x100,
  ITEM_IronPickaxe         = 0x101,
  ITEM_IronAxe             = 0x102,
  ITEM_FlintAndSteel       = 0x103,
  ITEM_Apple               = 0x104,
  ITEM_Bow                 = 0x105,
  ITEM_ArrowItem           = 0x106,
  ITEM_Coal                = 0x107,
  ITEM_Diamond             = 0x108,
  ITEM_IronIngot           = 0x109,
  ITEM_GoldIngot           = 0x10A,
  ITEM_IronSword           = 0x10B,
  ITEM_WoodenSword         = 0x10C,
  ITEM_WoodenShovel        = 0x10D,
  ITEM_WoodenPickaxe       = 0x10E,
  ITEM_WoodenAxe           = 0x10F,
  ITEM_StoneSword          = 0x110,
  ITEM_StoneShovel         = 0x111,
  ITEM_StonePickaxe        = 0x112,
  ITEM_StoneAxe            = 0x113,
  ITEM_DiamondSword        = 0x114,
  ITEM_DiamondShovel       = 0x115,
  ITEM_DiamondPickaxe      = 0x116,
  ITEM_DiamondAxe          = 0x117,
  ITEM_Stick               = 0x118,
  ITEM_Bowl                = 0x119,
  ITEM_MushroomSoup        = 0x11A,
  ITEM_GoldSword           = 0x11B,
  ITEM_GoldShovel          = 0x11C,
  ITEM_GoldPickaxe         = 0x11D,
  ITEM_GoldAxe             = 0x11E,
  ITEM_StringItem          = 0x11F,
  ITEM_Feather             = 0x120,
  ITEM_Gunpowder           = 0x121,
  ITEM_WoodenHoe           = 0x122,
  ITEM_StoneHoe            = 0x123,
  ITEM_IronHoe             = 0x124,
  ITEM_DiamondHoe          = 0x125,
  ITEM_GoldHoe             = 0x126,
  ITEM_Seeds               = 0x127,
  ITEM_Wheat               = 0x128,
  ITEM_Bread               = 0x129,
  ITEM_LeatherHelmet       = 0x12A,
  ITEM_LeatherChestPlate   = 0x12B,
  ITEM_LeatherLeggings     = 0x12C,
  ITEM_LeatherBoots        = 0x12D,
  ITEM_ChainmailHelmet     = 0x12E,
  ITEM_ChainmailChestPlate = 0x12F,
  ITEM_ChainmailLeggings   = 0x130,
  ITEM_ChainmailBoots      = 0x131,
  ITEM_IronHelmet          = 0x132,
  ITEM_IronChestPlate      = 0x133,
  ITEM_IronLeggings        = 0x134,
  ITEM_IronBoots           = 0x135,
  ITEM_DiamondHelmet       = 0x136,
  ITEM_DiamondChestPlate   = 0x137,
  ITEM_DiamondLeggings     = 0x138,
  ITEM_DiamondBoots        = 0x139,
  ITEM_GoldHelmet          = 0x13A,
  ITEM_GoldChestPlate      = 0x13B,
  ITEM_GoldLeggings        = 0x13C,
  ITEM_GoldBoots           = 0x13D,
  ITEM_Flint               = 0x13E,
  ITEM_RawPorkchop         = 0x13F,
  ITEM_CookedPortchop      = 0x140,
  ITEM_PaintingItem        = 0x141,
  ITEM_GoldApple           = 0x142,
  ITEM_Sign                = 0x143,
  ITEM_WoodenDoorBlock     = 0x144,
  ITEM_Bucket              = 0x145,
  ITEM_BucketWithWater     = 0x146,
  ITEM_BucketWithLava      = 0x147,
  ITEM_MineCart            = 0x148,
  ITEM_Saddle              = 0x149,
  ITEM_IronDoor            = 0x14A,
  ITEM_Redstone            = 0x14B,
  ITEM_Snowball            = 0x14C,
  ITEM_BoatItem            = 0x14D,
  ITEM_Leather             = 0x14E,
  ITEM_BucketWithMilk      = 0x14F,
  ITEM_ClayBrick           = 0x150,
  ITEM_ClayBalls           = 0x151,
  ITEM_SugarCane           = 0x152,
  ITEM_Paper               = 0x153,
  ITEM_Book                = 0x154,
  ITEM_Slimeball           = 0x155,
  ITEM_StorageMinecart     = 0x156,
  ITEM_PoweredMinecart     = 0x157,
  ITEM_Egg                 = 0x158,
  ITEM_Compass             = 0x159,
  ITEM_FishingRod          = 0x15A,
  ITEM_Clock               = 0x15B,
  ITEM_GlowstoneDust       = 0x15C,
  ITEM_RawFish             = 0x15D,
  ITEM_CookedFish          = 0x15E,
  ITEM_Dye                 = 0x15F,
  ITEM_Bone                = 0x160,
  ITEM_Sugar               = 0x161,
  ITEM_Cake                = 0x162,
  ITEM_BedBlock            = 0x163,
  ITEM_RedstoneRepeater    = 0x164,
  ITEM_GoldMusicDisc       = 0x8D0,
  ITEM_GreenMusicDisc      = 0x8D1,

  BLOCK_Air                 = 0x00,
  BLOCK_Stone               = 0x01,
  BLOCK_Grass               = 0x02,
  BLOCK_Dirt                = 0x03,
  BLOCK_Cobblestone         = 0x04,
  BLOCK_WoodenPlank         = 0x05,
  BLOCK_Sapling             = 0x06,
  BLOCK_Bedrock             = 0x07,
  BLOCK_Water               = 0x08,
  BLOCK_StationaryWater     = 0x09,
  BLOCK_Lava                = 0x0A,
  BLOCK_StationaryLava      = 0x0B,
  BLOCK_Sand                = 0x0C,
  BLOCK_Gravel              = 0x0D,
  BLOCK_GoldOre             = 0x0E,
  BLOCK_IronOre             = 0x0F,
  BLOCK_CoalOre             = 0x10,
  BLOCK_Wood                = 0x11,
  BLOCK_Leaves              = 0x12,
  BLOCK_Sponge              = 0x13,
  BLOCK_Glass               = 0x14,
  BLOCK_LapisLazuliOre      = 0x15,
  BLOCK_LapisLazuliBlock    = 0x16,
  BLOCK_DispenserBlock      = 0x17,
  BLOCK_Sandstone           = 0x18,
  BLOCK_Note                = 0x19,
  BLOCK_Bed                 = 0x1A,
  BLOCK_Wool                = 0x23,
  BLOCK_YellowFlower        = 0x25,
  BLOCK_RedRose             = 0x26,
  BLOCK_BrownMushroom       = 0x27,
  BLOCK_RedMushroom         = 0x28,
  BLOCK_GoldBlock           = 0x29,
  BLOCK_IronBlock           = 0x2A,
  BLOCK_DoubleSlab          = 0x2B,
  BLOCK_Slab                = 0x2C,
  BLOCK_BrickBlock          = 0x2D,
  BLOCK_TNT                 = 0x2E,
  BLOCK_Bookshelf           = 0x2F,
  BLOCK_MossStone           = 0x30,
  BLOCK_Obsidian            = 0x31,
  BLOCK_Torch               = 0x32,
  BLOCK_Fire                = 0x33,
  BLOCK_MonsterSpawner      = 0x34,
  BLOCK_WoodenStairs        = 0x35,
  BLOCK_ChestBlock          = 0x36,
  BLOCK_RedstoneWire        = 0x37,
  BLOCK_DiamondOre          = 0x38,
  BLOCK_DiamondBlock        = 0x39,
  BLOCK_CraftingTable       = 0x3A,
  BLOCK_Crops               = 0x3B,
  BLOCK_Farmland            = 0x3C,
  BLOCK_FurnaceBlock        = 0x3E,
  BLOCK_SignPost            = 0x3F,
  BLOCK_WoodenDoor          = 0x40,
  BLOCK_Ladder              = 0x41,
  BLOCK_Rails               = 0x42,
  BLOCK_CobblestoneStairs   = 0x43,
  BLOCK_WallSign            = 0x44,
  BLOCK_Lever               = 0x45,
  BLOCK_StonePressurePlate  = 0x46,
  BLOCK_IronDoorBlock       = 0x47,
  BLOCK_WoodenPressurePlate = 0x48,
  BLOCK_RedstoneOre         = 0x49,
  BLOCK_GlowingRedstoneOre  = 0x4A,
  BLOCK_RedstoneTorchOff    = 0x4B,
  BLOCK_RedstoneTorchOn     = 0x4C,
  BLOCK_StoneButton         = 0x4D,
  BLOCK_Snow                = 0x4E,
  BLOCK_Ice                 = 0x4F,
  BLOCK_SnowBlock           = 0x50,
  BLOCK_Cactus              = 0x51,
  BLOCK_ClayBlock           = 0x52,
  BLOCK_SugarCaneBlock      = 0x53,
  BLOCK_Jukebox             = 0x54,
  BLOCK_Fence               = 0x55,
  BLOCK_Pumpkin             = 0x56,
  BLOCK_Netherrack          = 0x57,
  BLOCK_SoulSand            = 0x58,
  BLOCK_GlowstoneBlock      = 0x59,
  BLOCK_Portal              = 0x5A,
  BLOCK_JackOLantern        = 0x5B,
  BLOCK_CackeBlock          = 0x5C,
  BLOCK_RedstoneRepeaterOff = 0x5D,
  BLOCK_RedstoneRepeaterOn  = 0x5E
};

/*********** Info Types ***********/

struct PacketInfo
{
  inline operator size_t() const { return size_t(code); }
  inline bool operator==(const PacketInfo & other) const { return code == other.code; }

  EPacketNames code;
  size_t size;
  std::string name;
};

struct BlockItemInfo
{
  enum Type { BLOCK = 0, ITEM = 1 };

  inline Type type() const { return code < 256 ? BLOCK : ITEM ; }
  inline operator size_t() const { return size_t(code); }
  inline bool operator==(const BlockItemInfo & other) const { return code == other.code; }

  EBlockItem code;
  std::string name;
};

extern std::unordered_set<PacketInfo, std::hash<size_t>> PACKET_INFO;

extern std::unordered_set<BlockItemInfo, std::hash<size_t>> BLOCKITEM_INFO;



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
