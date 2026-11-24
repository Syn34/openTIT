// openTIT - Level AI behaviours: the controller classes the AI component
// builds (see AIComponent.h for the type hashes).
//
// Class names are descriptive guesses: "Critter" and "Player" are the type
// names; El Jefe, Ms Decibel, Bismarque and Paradox are named after the
// tuning variables and level objects their code uses; the rest after what
// their settings describe.
#pragma once

#include "sly/levels/ai/AIController.h"

namespace sly {
namespace levels {
namespace ai {

// Ambient critters (vtable 0x00B0E028, 0x1A0 bytes; settings 0x00F94784..).
class CritterAI : public AIController {
public:
    uint32_t field_198 = 0;    // +0x198
    uint32_t field_19C = 0;    // +0x19C
};

// Player buddies driven by the AI (vtable 0x00B0E1B8, 0x1B0 bytes; settings
// FollowDist / AttackDist / Murray / Bentley).
class PlayerAI : public AIController {
public:
};

// Boss that runs state patterns per level and spawns ghosts (vtable
// 0x00B0E2F0, 0x180 bytes).
class PatternBossAI : public AIController {
public:
};

// Scripted opponent ("CPU"; vtable 0x00B0E428, 0x170 bytes).
class CpuAI : public AIController {
public:
};

// Standard enemy (vtable 0x00B0E5B8, 0x1E0 bytes): patrol, cover, turret and
// combat settings.
class EnemyAI : public AIController {
public:
};

// Watcher with awareness, vision and detection cylinder settings (vtable
// 0x00B0E750, 0x150 bytes) and a variant of it (vtable 0x00B0EDA0, 0x1F0
// bytes; its code lies outside this range).
class SentryAI : public AIController {
public:
};
class SentryAIVariant : public SentryAI {
public:
};

// Bosses (vtables 0x00B0E888, 0x00B0E9C0, 0x00B0EAF8, 0x00B0EC30).
class BismarqueAI : public AIController {
public:
};
class ElJefeAI : public AIController {
public:
};
class MsDecibelAI : public AIController {
public:
    bool field_1870 = false;   // +0x1870
};
class ParadoxAI : public AIController {
public:
};

} // namespace ai
} // namespace levels
} // namespace sly
