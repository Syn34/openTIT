// openTIT - The minigame classes (all derived from Minigame, vtables
// 0x00B20338..0x00B20C30): only their destructors and the small virtual
// overrides in 0x0077AF08-0x0077F4E8 are ported. Their constructors,
// updates and message handlers live with the minigame code at
// 0x003A0000-0x00420000 and are not ported; members are only modelled as far
// as the destructors need them (array element types are not decoded).
#pragma once

#include "sly/gameplay/Minigame.h"
#include "sly/gameplay/PenguinMinigames.h"
#include "ape/memory/ApeTArray.h"

#include <vector>

namespace sly {
namespace gameplay {

// The fishing minigame (vtable 0x00B20428, constructor 0x003B3148).
class FishingMinigame : public Minigame {
public:
    ~FishingMinigame() override;
    ape::ApeTArray<void*> field_38;   // +0x38
};

// Minigame of vtable 0x00B20488 (constructor 0x003B835C).
// NOTE: which game it is is not known.
class MinigameB20488 : public Minigame {
public:
    ~MinigameB20488() override;
    ape::ApeTArray<void*> field_68;   // +0x68
};

// Minigame of vtable 0x00B204E8 (constructor 0x003C6CC0): three extra
// queries that answer "no". NOTE: meanings unknown.
class MinigameB204E8 : public Minigame {
public:
    virtual bool Slot22();
    virtual bool Slot23();
    virtual bool Slot24();
};

// The quick time event minigame (vtable 0x00B20558, constructor 0x003D3704).
class QteMinigame : public Minigame {
public:
    ~QteMinigame() override;
    std::vector<uint8_t> field_40;    // +0x40 compact array
    std::vector<uint8_t> field_58;    // +0x58 compact array
    ape::ApeTArray<void*> field_70;   // +0x70
    std::vector<uint8_t> field_170;   // +0x170 compact array
    ape::ApeTArray<void*> field_188;  // +0x188
};

// Game-side event listener base (vtable 0x00B21A30, destructor 0x0048F6B0):
// leaving every listener group is all its destructor does.
class MinigameListener {
public:
    virtual ~MinigameListener();
    // Slot 2: a message posted to a group the listener joined.
    virtual void OnListenerMessage(const void* message) { (void)message; }
    bool joined = false;   // +0x04 the listener is in some group
};

// The fencing minigame (vtable 0x00B205B8 + listener 0x00B2061C at +0x38;
// constructor 0x003D8B34).
class FencingMinigame : public Minigame, public MinigameListener {
public:
    ~FencingMinigame() override;
    // Slot 14: answers "no" (the base says yes).
    bool Slot14() override;
};

// Minigame of vtable 0x00B20630 (constructor 0x003DCCB4, scripted UI
// "Scripts/UI/%s.xml").
class MinigameB20630 : public Minigame {
public:
    // Slot 16: true while the global value 0x010854AC is not above zero.
    // NOTE: the value's meaning is unknown.
    bool Slot16() override;
};

// Minigame of vtable 0x00B206A8 (constructor 0x0047C8A0).
class MinigameB206A8 : public Minigame {
public:
    ~MinigameB206A8() override;
    // Slot 17: true unless the byte at +0x917 is set.
    bool Slot17() override;
    uint8_t field_917 = 0;             // +0x917
    ape::ApeTArray<void*> field_924;   // +0x924
};

// Minigame of vtable 0x00B20748 (constructor 0x003F02B8).
class MinigameB20748 : public Minigame {
public:
    ~MinigameB20748() override;
    ape::ApeTArray<void*> field_70;    // +0x70
    ape::ApeTArray<void*> field_84;    // +0x84
    ape::ApeTArray<void*> field_E8;    // +0xE8
};

// The penguin montage (vtable 0x00B20C30, constructor 0x0041F1FC).
class PenguinMontageMinigame : public Minigame {
public:
    ~PenguinMontageMinigame() override;
    ape::ApeTArray<void*> field_48;    // +0x48
};

// Base of the six penguin minigames ("Getting Stronger", vtable
// 0x00B207A8): slots 22-34 with defaults. NOTE: names unknown; slots
// 22 and 25-27 report four per-game flags, 29/30 the byte +0x36.
class PenguinMinigame : public Minigame {
public:
    ~PenguinMinigame() override;
    virtual bool Slot22();
    virtual void Slot23();
    virtual void Slot24();
    virtual bool Slot25();
    virtual bool Slot26();
    virtual bool Slot27();
    virtual void Slot28();
    virtual bool Slot29();
    virtual void Slot30(bool value);
    virtual void Slot31();
    virtual void Slot32();
    virtual void Slot33();
    virtual bool Slot34();
};

// Penguin balance (vtable 0x00B20850, constructor 0x003F834C).
class PenguinBalanceMinigame : public PenguinMinigame {
public:
    ~PenguinBalanceMinigame() override;
    bool Slot22() override;
    bool Slot25() override;
    bool Slot26() override;
    bool Slot27() override;
    bool Slot29() override;
    ape::ApeTArray<void*> field_110;   // +0x110
    uint8_t flags[4] = {};             // +0x168
};

// Penguin dodge (vtable 0x00B208F8, constructor 0x004015D0).
class PenguinDodgeMinigame : public PenguinMinigame {
public:
    ~PenguinDodgeMinigame() override;
    bool Slot22() override;
    bool Slot25() override;
    bool Slot26() override;
    bool Slot27() override;
    ape::ApeTArray<void*> field_68;    // +0x68
    uint8_t flags[4] = {};             // +0xEC
};

// Penguin punch (vtable 0x00B209A0, constructor 0x0040773C).
class PenguinPunchMinigame : public PenguinMinigame {
public:
    ~PenguinPunchMinigame() override;
    bool Slot22() override;
    bool Slot25() override;
    bool Slot26() override;
    bool Slot27() override;
    ape::ApeTArray<void*> field_64;    // +0x64
    uint8_t flags[4] = {};             // +0xE4
};

// Penguin slingshot (vtable 0x00B20A48, constructor 0x0040F244): owns two
// spawned entities (+0x14C, +0x150) that leave the world with it.
class PenguinSlingshotMinigame : public PenguinMinigame {
public:
    ~PenguinSlingshotMinigame() override;
    bool Slot22() override;
    bool Slot25() override;
    bool Slot26() override;
    bool Slot27() override;
    ape::ApeTArray<void*> field_A4;    // +0xA4
    WorldObject* spawned[2] = {};      // +0x14C
    uint8_t flags[4] = {};             // +0x1DC
};

// Penguin sumo (vtable 0x00B20AF0, constructor 0x00416FA4): the penguin's
// record is a member (+0x6C).
class PenguinSumoMinigame : public PenguinMinigame {
public:
    ~PenguinSumoMinigame() override;
    bool Slot25() override;
    bool Slot26() override;
    bool Slot27() override;
    PenguinSumoRecord penguin;         // +0x6C
    uint8_t flags[3] = {};             // +0x1A8
};

// Whack-a-penguin (vtable 0x00B20B98, constructor 0x0041BB54).
class WhackPenguinMinigame : public PenguinMinigame {
public:
    ~WhackPenguinMinigame() override;
    bool Slot22() override;
    bool Slot25() override;
    bool Slot26() override;
    bool Slot27() override;
    ape::ApeTArray<void*> field_68;    // +0x68
    uint8_t flags[4] = {};             // +0x1E0
};

} // namespace gameplay
} // namespace sly
