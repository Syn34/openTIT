// openTIT - Typed component accessors of entities (0x000D935C-0x000D9614).
//
// The console has one small function per component type that calls
// Entity::GetComponent(type). Names follow the component's known role where
// the callers make it clear; the others keep the type number.
#include "sly/entity/Entity.h"

namespace sly {
namespace entity {

// @func 0x000D935C EntityComponent12
EntityComponent* EntityComponent12(const Entity* e) { return e->GetComponent(kComp_12); }
// @func 0x000D93AC EntityComponent1F
EntityComponent* EntityComponent1F(const Entity* e) { return e->GetComponent(kComp_Center); }
// @func 0x000D93D4 EntityComponent01
EntityComponent* EntityComponent01(const Entity* e) { return e->GetComponent(kComp_01); }
// @func 0x000D93FC EntityComponent03
EntityComponent* EntityComponent03(const Entity* e) { return e->GetComponent(kComp_03); }
// @func 0x000D9424 EntityComponent0E
EntityComponent* EntityComponent0E(const Entity* e) { return e->GetComponent(kComp_0E); }
// @func 0x000D9474 EntityComponent11
EntityComponent* EntityComponent11(const Entity* e) { return e->GetComponent(kComp_11); }
// @func 0x000D949C EntityTargeting
EntityComponent* EntityTargeting(const Entity* e) { return e->GetComponent(kComp_Targeting); }
// @func 0x000D94CC EntityGadget
EntityComponent* EntityGadget(const Entity* e) { return e->GetComponent(kComp_Gadget); }
// @func 0x000D94F4 EntitySkin
EntityComponent* EntitySkin(const Entity* e) { return e->GetComponent(kComp_Skin); }
// @func 0x000D951C EntityInteraction
EntityComponent* EntityInteraction(const Entity* e) { return e->GetComponent(kComp_Interaction); }
// @func 0x000D9544 EntityCarry
EntityComponent* EntityCarry(const Entity* e) { return e->GetComponent(kComp_Carry); }
// @func 0x000D956C EntityComponent14
EntityComponent* EntityComponent14(const Entity* e) { return e->GetComponent(kComp_14); }
// @func 0x000D9594 EntityComponent20
EntityComponent* EntityComponent20(const Entity* e) { return e->GetComponent(kComp_20); }
// @func 0x000D95C4 EntityComponent25
EntityComponent* EntityComponent25(const Entity* e) { return e->GetComponent(kComp_25); }
// @func 0x000D95EC EntityComponent1E
EntityComponent* EntityComponent1E(const Entity* e) { return e->GetComponent(kComp_1E); }
// @func 0x000D9614 EntityComponent04
EntityComponent* EntityComponent04(const Entity* e) { return e->GetComponent(kComp_04); }

// Same lookup as virtual slot 15, for callers holding a const entity.
EntityComponent* EntityCharacterComp(const Entity* e) { return e->GetComponent(kComp_Character); }

} // namespace entity
} // namespace sly
