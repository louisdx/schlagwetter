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

PacketInfoMap PACKET_INFO = {
  { PACKET_KEEP_ALIVE,               { 0,                   "keep-alive"} },
  { PACKET_LOGIN_REQUEST,            { PACKET_VARIABLE_LEN, "login request"} },
  { PACKET_HANDSHAKE,                { PACKET_VARIABLE_LEN, "handshake"} },
  { PACKET_PRE_CHUNK,                { 9,                   "chunk request"} },
  { PACKET_CHAT_MESSAGE,             { PACKET_VARIABLE_LEN, "chat message"} },
  { PACKET_USE_ENTITY,               { 9,                   "use entity"} },
  { PACKET_PLAYER,                   { 1,                   "player"} },
  { PACKET_PLAYER_POSITION,          { 33,                  "player position"} },
  { PACKET_PLAYER_LOOK,              { 9,                   "player look"} },
  { PACKET_PLAYER_POSITION_AND_LOOK, { 41,                  "player pos+look"} },
  { PACKET_PLAYER_DIGGING,           { 11,                  "player digging"} },
  { PACKET_PLAYER_BLOCK_PLACEMENT,   { PACKET_VARIABLE_LEN, "player block placement"} },
  { PACKET_HOLDING_CHANGE,           { 2,                   "player holding change"} },
  { PACKET_ARM_ANIMATION,            { 5,                   "arm animation"} },
  { PACKET_PICKUP_SPAWN,             { 22,                  "pickup spawn"} },
  { PACKET_DISCONNECT,               { PACKET_VARIABLE_LEN, "disconnect"} },
  { PACKET_RESPAWN,                  { 0,                   "respawn"} },
  { PACKET_INVENTORY_CHANGE,         { PACKET_VARIABLE_LEN, "inventory change"} },
  { PACKET_INVENTORY_CLOSE,          { 1,                   "inventory close"} },
  { PACKET_SIGN,                     { PACKET_VARIABLE_LEN, "sign"} },
  { PACKET_TRANSACTION,              { 4,                   "transaction"} },
  { PACKET_ENTITY_CROUCH,            { 5,                   "entity crouch"} }
};

BlockItemInfoMap BLOCKITEM_INFO = {
  { ITEM_WoodenShovel,     BlockItemInfo("Wooden Shovel") },
  { ITEM_WoodenPickaxe,    BlockItemInfo("Wooden Pickaxe") },
  { ITEM_WoodenHoe,        BlockItemInfo("Wooden Hoe") },
  { ITEM_WoodenAxe,        BlockItemInfo("Wooden Hatchet") },
  { ITEM_WoodenSword,      BlockItemInfo("Wooden Sword") },
  { ITEM_StoneShovel,      BlockItemInfo("Stone Shovel") },
  { ITEM_StonePickaxe,     BlockItemInfo("Stone Pickaxe") },
  { ITEM_StoneHoe,         BlockItemInfo("Stone Hoe") },
  { ITEM_StoneAxe,         BlockItemInfo("Stone Hatchet") },
  { ITEM_StoneSword,       BlockItemInfo("Stone Sword") },
  { ITEM_IronShovel,       BlockItemInfo("Iron Shovel") },
  { ITEM_IronPickaxe,      BlockItemInfo("Iron Pickaxe") },
  { ITEM_IronHoe,          BlockItemInfo("Iron Hoe") },
  { ITEM_IronAxe,          BlockItemInfo("Iron Hatchet") },
  { ITEM_IronSword,        BlockItemInfo("Iron Sword") },
  { ITEM_DiamondShovel,    BlockItemInfo("Diamond Shovel") },
  { ITEM_DiamondPickaxe,   BlockItemInfo("Diamond Pickaxe") },
  { ITEM_DiamondHoe,       BlockItemInfo("Diamond Hoe") },
  { ITEM_DiamondAxe,       BlockItemInfo("Diamond Hatchet") },
  { ITEM_DiamondSword,     BlockItemInfo("Diamond Sword") },
  { ITEM_GoldShovel,       BlockItemInfo("Golden Shovel") },
  { ITEM_GoldPickaxe,      BlockItemInfo("Golden Pickaxe") },
  { ITEM_GoldHoe,          BlockItemInfo("Golden Hoe") },
  { ITEM_GoldAxe,          BlockItemInfo("Golden Hatchet") },
  { ITEM_GoldSword,        BlockItemInfo("Golden Sword") },
  { ITEM_FlintAndSteel,    BlockItemInfo("Flint and Steel") },
  { ITEM_Apple,            BlockItemInfo("Apple") },
  { ITEM_Bow,              BlockItemInfo("Bow") },
  { ITEM_ArrowItem,        BlockItemInfo("Arrow") },
  { ITEM_Coal,             BlockItemInfo("Coal") },
  { ITEM_Diamond,          BlockItemInfo("Diamond") },
  { ITEM_IronIngot,        BlockItemInfo("Iron Ingot") },
  { ITEM_GoldIngot,        BlockItemInfo("Gold Ingot") },
  { ITEM_Stick,            BlockItemInfo("Stick") },
  { ITEM_Bowl,             BlockItemInfo("Bowl") },
  { ITEM_MushroomSoup,     BlockItemInfo("Mushroom Soup") },
  { ITEM_StringItem,       BlockItemInfo("String") },
  { ITEM_Feather,          BlockItemInfo("Feather") },
  { ITEM_Gunpowder,        BlockItemInfo("Gunpowder") },
  { ITEM_Seeds,            BlockItemInfo("Seeds") },
  { ITEM_Wheat,            BlockItemInfo("Wheat") },
  { ITEM_Bread,            BlockItemInfo("Bread") },
  { ITEM_LeatherHelmet,       BlockItemInfo("Leather Helmet") },
  { ITEM_LeatherChestPlate,   BlockItemInfo("Leather Chest Plate") },
  { ITEM_LeatherLeggings,     BlockItemInfo("Leather Leggings") },
  { ITEM_LeatherBoots,        BlockItemInfo("Leather Boots") },
  { ITEM_ChainmailHelmet,     BlockItemInfo("Chainmail Helmet") },
  { ITEM_ChainmailChestPlate, BlockItemInfo("Chainmail Chest Plate") },
  { ITEM_ChainmailLeggings,   BlockItemInfo("Chainmail Leggings") },
  { ITEM_ChainmailBoots,      BlockItemInfo("Chainmail Boots") },
  { ITEM_IronHelmet,          BlockItemInfo("Iron Helmet") },
  { ITEM_IronChestPlate,      BlockItemInfo("Iron Chest Plate") },
  { ITEM_IronLeggings,     BlockItemInfo("") },
  { ITEM_IronBoots,     BlockItemInfo("") },
  { ITEM_DiamondHelmet,     BlockItemInfo("") },
  { ITEM_DiamondChestPlate,     BlockItemInfo("") },
  { ITEM_DiamondLeggings,     BlockItemInfo("") },
  { ITEM_DiamondBoots,     BlockItemInfo("") },
  { ITEM_GoldHelmet,     BlockItemInfo("") },
  { ITEM_GoldChestPlate,     BlockItemInfo("") },
  { ITEM_GoldLeggings,     BlockItemInfo("") },
  { ITEM_GoldBoots,     BlockItemInfo("") },
  { ITEM_Flint,     BlockItemInfo("") },
  { ITEM_RawPorkchop,     BlockItemInfo("") },
  { ITEM_CookedPortchop,     BlockItemInfo("") },
  { ITEM_PaintingItem,     BlockItemInfo("") },
  { ITEM_GoldApple,     BlockItemInfo("") },
  { ITEM_Sign,     BlockItemInfo("") },
  { ITEM_WoodenDoor,     BlockItemInfo("Wooden Door (item)") },
  { ITEM_Bucket,     BlockItemInfo("") },
  { ITEM_BucketWithWater,     BlockItemInfo("") },
  { ITEM_BucketWithLava,     BlockItemInfo("") },
  { ITEM_MineCart,     BlockItemInfo("") },
  { ITEM_Saddle,     BlockItemInfo("") },
  { ITEM_IronDoor,     BlockItemInfo("Iron Door (item)") },
  { ITEM_Redstone,     BlockItemInfo("") },
  { ITEM_Snowball,     BlockItemInfo("") },
  { ITEM_BoatItem,     BlockItemInfo("") },
  { ITEM_Leather,     BlockItemInfo("") },
  { ITEM_BucketWithMilk,     BlockItemInfo("") },
  { ITEM_ClayBrick,     BlockItemInfo("") },
  { ITEM_ClayBalls,     BlockItemInfo("") },
  { ITEM_SugarCane,     BlockItemInfo("") },
  { ITEM_Paper,     BlockItemInfo("") },
  { ITEM_Book,     BlockItemInfo("") },
  { ITEM_Slimeball,     BlockItemInfo("") },
  { ITEM_StorageMinecart,     BlockItemInfo("") },
  { ITEM_PoweredMinecart,     BlockItemInfo("") },
  { ITEM_Egg,     BlockItemInfo("") },
  { ITEM_Compass,     BlockItemInfo("") },
  { ITEM_FishingRod,     BlockItemInfo("") },
  { ITEM_Clock,     BlockItemInfo("") },
  { ITEM_GlowstoneDust,     BlockItemInfo("") },
  { ITEM_RawFish,     BlockItemInfo("") },
  { ITEM_CookedFish,     BlockItemInfo("") },
  { ITEM_Dye,     BlockItemInfo("") },
  { ITEM_Bone,     BlockItemInfo("") },
  { ITEM_Sugar,     BlockItemInfo("") },
  { ITEM_Cake,     BlockItemInfo("") },
  { ITEM_BedBlock,     BlockItemInfo("") },
  { ITEM_RedstoneRepeater,     BlockItemInfo("") },
  { ITEM_GoldMusicDisc,     BlockItemInfo("") },
  { ITEM_GreenMusicDisc,     BlockItemInfo("") },

  { BLOCK_Air,     BlockItemInfo("Air") },
  { BLOCK_Stone,     BlockItemInfo("Stone") },
  { BLOCK_Grass,     BlockItemInfo("Grass") },
  { BLOCK_Dirt,     BlockItemInfo("Dirt") },
  { BLOCK_Cobblestone,     BlockItemInfo("Cobblestonr") },
  { BLOCK_WoodenPlank,     BlockItemInfo("Wooden Plank") },
  { BLOCK_Sapling,     BlockItemInfo("Sapling") },
  { BLOCK_Bedrock,     BlockItemInfo("Bedrock") },
  { BLOCK_Water,     BlockItemInfo("Water") },
  { BLOCK_StationaryWater,     BlockItemInfo("?Water") },
  { BLOCK_Lava,     BlockItemInfo("Lava") },
  { BLOCK_StationaryLava,     BlockItemInfo("?Lava") },
  { BLOCK_Sand,     BlockItemInfo("Sand") },
  { BLOCK_Gravel,     BlockItemInfo("Gravel") },
  { BLOCK_GoldOre,     BlockItemInfo("Gold Ore") },
  { BLOCK_IronOre,     BlockItemInfo("Iron Ore") },
  { BLOCK_CoalOre,     BlockItemInfo("Coal Ore") },
  { BLOCK_Wood,     BlockItemInfo("Wood") },
  { BLOCK_Leaves,     BlockItemInfo("Leaves") },
  { BLOCK_Sponge,     BlockItemInfo("Sponge") },
  { BLOCK_Glass,     BlockItemInfo("Glass") },
  { BLOCK_LapisLazuliOre,     BlockItemInfo("Lapis Lazuli Ore") },
  { BLOCK_LapisLazuliBlock,     BlockItemInfo("Lapis Lazuli Block") },
  { BLOCK_DispenserBlock,     BlockItemInfo("Dispenser") },
  { BLOCK_Sandstone,     BlockItemInfo("Sandstone") },
  { BLOCK_Note,     BlockItemInfo("Noteblock") },
  { BLOCK_Bed,     BlockItemInfo("Bed") },
  { BLOCK_Wool,     BlockItemInfo("Wool") },
  { BLOCK_YellowFlower,     BlockItemInfo("Daffodil") },
  { BLOCK_RedRose,     BlockItemInfo("Rose") },
  { BLOCK_BrownMushroom,     BlockItemInfo("Brown Mushroom") },
  { BLOCK_RedMushroom,     BlockItemInfo("Red Mushroom") },
  { BLOCK_GoldBlock,     BlockItemInfo("Gold Block") },
  { BLOCK_IronBlock,     BlockItemInfo("Iron Block") },
  { BLOCK_DoubleSlab,     BlockItemInfo("Double Slab") },
  { BLOCK_Slab,     BlockItemInfo("Slab?") },
  { BLOCK_BrickBlock,     BlockItemInfo("Brick Block") },
  { BLOCK_TNT,     BlockItemInfo("TNT") },
  { BLOCK_Bookshelf,     BlockItemInfo("Bookshelf") },
  { BLOCK_MossStone,     BlockItemInfo("") },
  { BLOCK_Obsidian,     BlockItemInfo("") },
  { BLOCK_Torch,     BlockItemInfo("") },
  { BLOCK_Fire,     BlockItemInfo("") },
  { BLOCK_MonsterSpawner,     BlockItemInfo("") },
  { BLOCK_WoodenStairs,     BlockItemInfo("") },
  { BLOCK_ChestBlock,     BlockItemInfo("") },
  { BLOCK_RedstoneWire,     BlockItemInfo("") },
  { BLOCK_DiamondOre,     BlockItemInfo("") },
  { BLOCK_DiamondBlock,     BlockItemInfo("") },
  { BLOCK_CraftingTable,     BlockItemInfo("") },
  { BLOCK_Crops,     BlockItemInfo("") },
  { BLOCK_Farmland,     BlockItemInfo("") },
  { BLOCK_FurnaceBlock,     BlockItemInfo("") },
  { BLOCK_SignPost,     BlockItemInfo("") },
  { BLOCK_WoodenDoor,     BlockItemInfo("Wooden Door") },
  { BLOCK_Ladder,     BlockItemInfo("") },
  { BLOCK_Rails,     BlockItemInfo("") },
  { BLOCK_CobblestoneStairs,     BlockItemInfo("") },
  { BLOCK_WallSign,     BlockItemInfo("") },
  { BLOCK_Lever,     BlockItemInfo("") },
  { BLOCK_WoodenPressurePlate,    BlockItemInfo("Wooden Pressure Plate") },
  { BLOCK_StonePressurePlate,     BlockItemInfo("Stone Pressure Plate") },
  { BLOCK_IronDoor,               BlockItemInfo("Iron Door") },
  { BLOCK_RedstoneOre,     BlockItemInfo("") },
  { BLOCK_GlowingRedstoneOre,     BlockItemInfo("") },
  { BLOCK_RedstoneTorchOff,     BlockItemInfo("") },
  { BLOCK_RedstoneTorchOn,     BlockItemInfo("") },
  { BLOCK_StoneButton,     BlockItemInfo("") },
  { BLOCK_Snow,     BlockItemInfo("") },
  { BLOCK_Ice,     BlockItemInfo("") },
  { BLOCK_SnowBlock,     BlockItemInfo("") },
  { BLOCK_Cactus,     BlockItemInfo("") },
  { BLOCK_ClayBlock,     BlockItemInfo("") },
  { BLOCK_SugarCaneBlock,     BlockItemInfo("") },
  { BLOCK_Jukebox,     BlockItemInfo("") },
  { BLOCK_Fence,     BlockItemInfo("") },
  { BLOCK_Pumpkin,     BlockItemInfo("") },
  { BLOCK_Netherrack,     BlockItemInfo("") },
  { BLOCK_SoulSand,     BlockItemInfo("") },
  { BLOCK_GlowstoneBlock,     BlockItemInfo("") },
  { BLOCK_Portal,     BlockItemInfo("") },
  { BLOCK_JackOLantern,     BlockItemInfo("") },
  { BLOCK_Cake,                  BlockItemInfo("Cake (block)") },
  { BLOCK_RedstoneRepeaterOff,   BlockItemInfo("Redstone Repeater (off)") },
  { BLOCK_RedstoneRepeaterOn,    BlockItemInfo("Redstone Repeater (on)") }
  };

PropertyMap EMIT_LIGHT(0, {
  { BLOCK_Lava, 15 },           // Lava
  { BLOCK_StationaryLava, 15 }, // Stationary Lava
  { BLOCK_BrownMushroom,  1 },  // Brown mushroom
  { BLOCK_Torch, 14 },          // Torch
  { BLOCK_Fire, 15 },           // Fire
  { BLOCK_FurnaceBlock, 14 },   // Lit furnace
  { 0x4A,  9 }, // Redstone ore (Glowing)
  { 0x4C,  7 }, // Redstone Torch (On)
  { 0x59, 15 }, // Lightstone
  { 0x5A, 11 }, // Portal
  { 0x5B, 15 }, // Jack-O-Lantern
} );

PropertyMap STOP_LIGHT(16, {
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


PropertyMap BLOCK_DIG_PROPERTIES(LEFTCLICK_DIGGABLE, {
  { BLOCK_Torch,      LEFTCLICK_REMOVABLE },
  { BLOCK_WoodenDoor, LEFTCLICK_DIGGABLE | LEFTCLICK_TRIGGER   },
  { BLOCK_IronDoor,   LEFTCLICK_DIGGABLE | LEFTCLICK_TRIGGER   },
} );

PropertyMap BLOCK_PLACEMENT_PROPERTIES(0, { } );

