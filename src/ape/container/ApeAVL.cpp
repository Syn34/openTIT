// openTIT - AVL<KeyType, ItemType> instantiations used by the engine.
//
// Each instantiation below corresponds to one set of template functions in
// the original executable (the console linker kept a private copy per
// key/item type). The addresses are listed per method.
#include "ape/container/ApeAVL.h"

#include <cstdint>

class hkpRigidBody;

namespace ape {

class ApeAnimScript;
class ApeAnimScriptCommand;
class ApeAnimScriptEvent;
class ApeAnimScriptLibrary;
class ApeAnimScriptState;
class ApeAnimScriptVar;
class ApeLevelDef;
class ApeLevelDefXMLLoader;

// AVL<unsigned int, unsigned int>
// @func 0x00739DD4 AVL<uint,uint>::RotateRight
// @func 0x00739F1C AVL<uint,uint>::RotateLeft
// @func 0x00749D40 AVL<uint,uint>::RotateRight (identical copy of a further, unidentified pointer-valued instantiation)
// @func 0x00749E88 AVL<uint,uint>::RotateLeft (identical copy of the same instantiation)
// @func 0x0073A064 AVL<uint,uint>::ReplaceWithPredecessor
// @func 0x0073A36C AVL<uint,uint>::RemoveFrom
// @func 0x0073AF5C AVL<uint,uint>::Search
// @func 0x0073AFC4 AVL<uint,uint>::Insert
// @func 0x0073B444 AVL<uint,uint>::Add
template class AVL<uint32_t, uint32_t>;

// AVL<unsigned int, hkpRigidBody*>
// @func 0x0076BE8C AVL<uint,hkpRigidBody*>::RotateRight
// @func 0x0076BFD4 AVL<uint,hkpRigidBody*>::RotateLeft
// @func 0x0076C11C AVL<uint,hkpRigidBody*>::ReplaceWithPredecessor
// @func 0x0076C424 AVL<uint,hkpRigidBody*>::RemoveFrom
// @func 0x0076D014 AVL<uint,hkpRigidBody*>::Search
// @func 0x0076D07C AVL<uint,hkpRigidBody*>::Insert
// @func 0x0076D4FC AVL<uint,hkpRigidBody*>::Add
template class AVL<uint32_t, hkpRigidBody*>;

// AVL<unsigned int, float>
// @func 0x0077B570 AVL<uint,float>::RotateRight
// @func 0x0077B6B8 AVL<uint,float>::RotateLeft
// @func 0x0077B800 AVL<uint,float>::ReplaceWithPredecessor
// @func 0x0077BB08 AVL<uint,float>::RemoveFrom
// @func 0x0077C6F8 AVL<uint,float>::Search
// @func 0x0077C760 AVL<uint,float>::Insert
// @func 0x0077CBE0 AVL<uint,float>::Add
template class AVL<uint32_t, float>;

// AVL<unsigned int, ApeAnimScriptCommand*>
// @func 0x007854B4 AVL<uint,ApeAnimScriptCommand*>::RotateRight
// @func 0x007855FC AVL<uint,ApeAnimScriptCommand*>::RotateLeft
// @func 0x00785744 AVL<uint,ApeAnimScriptCommand*>::Insert
// @func 0x00785BC4 AVL<uint,ApeAnimScriptCommand*>::Add
// @func 0x0078B218 AVL<uint,ApeAnimScriptCommand*>::ReplaceWithPredecessor
// @func 0x0078B520 AVL<uint,ApeAnimScriptCommand*>::RemoveFrom
// @func 0x0078C110 AVL<uint,ApeAnimScriptCommand*>::Search
template class AVL<uint32_t, ApeAnimScriptCommand*>;

// AVL<unsigned int, ApeAnimScriptVar*>
// @func 0x00785D58 AVL<uint,ApeAnimScriptVar*>::RotateRight
// @func 0x00785EA0 AVL<uint,ApeAnimScriptVar*>::RotateLeft
// @func 0x00785FE8 AVL<uint,ApeAnimScriptVar*>::Insert
// @func 0x00786468 AVL<uint,ApeAnimScriptVar*>::Add
// @func 0x0078D368 AVL<uint,ApeAnimScriptVar*>::ReplaceWithPredecessor
// @func 0x0078D670 AVL<uint,ApeAnimScriptVar*>::RemoveFrom
// @func 0x0078E260 AVL<uint,ApeAnimScriptVar*>::Search
template class AVL<uint32_t, ApeAnimScriptVar*>;

// AVL<unsigned int, ApeAnimScriptLibrary*>
// @func 0x0078A028 AVL<uint,ApeAnimScriptLibrary*>::RotateRight
// @func 0x0078A170 AVL<uint,ApeAnimScriptLibrary*>::RotateLeft
// @func 0x0078A2B8 AVL<uint,ApeAnimScriptLibrary*>::ReplaceWithPredecessor
// @func 0x0078A5C0 AVL<uint,ApeAnimScriptLibrary*>::RemoveFrom
// @func 0x0078B1B0 AVL<uint,ApeAnimScriptLibrary*>::Search
// @func 0x007918E4 AVL<uint,ApeAnimScriptLibrary*>::Insert
// @func 0x00791D64 AVL<uint,ApeAnimScriptLibrary*>::Add
template class AVL<uint32_t, ApeAnimScriptLibrary*>;

// AVL<unsigned int, ApeAnimScriptEvent*>
// @func 0x0078C178 AVL<uint,ApeAnimScriptEvent*>::RotateRight
// @func 0x0078C2C0 AVL<uint,ApeAnimScriptEvent*>::RotateLeft
// @func 0x0078C408 AVL<uint,ApeAnimScriptEvent*>::ReplaceWithPredecessor
// @func 0x0078C710 AVL<uint,ApeAnimScriptEvent*>::RemoveFrom
// @func 0x0078D300 AVL<uint,ApeAnimScriptEvent*>::Search
// @func 0x0078EB6C AVL<uint,ApeAnimScriptEvent*>::Insert
// @func 0x0078EFEC AVL<uint,ApeAnimScriptEvent*>::Add
template class AVL<uint32_t, ApeAnimScriptEvent*>;

// AVL<unsigned int, ApeAnimScriptState*>
// @func 0x0078E2C8 AVL<uint,ApeAnimScriptState*>::RotateRight
// @func 0x0078E410 AVL<uint,ApeAnimScriptState*>::RotateLeft
// @func 0x0078E558 AVL<uint,ApeAnimScriptState*>::Insert
// @func 0x0078E9D8 AVL<uint,ApeAnimScriptState*>::Add
// @func 0x00790984 AVL<uint,ApeAnimScriptState*>::ReplaceWithPredecessor
// @func 0x00790C8C AVL<uint,ApeAnimScriptState*>::RemoveFrom
// @func 0x0079187C AVL<uint,ApeAnimScriptState*>::Search
template class AVL<uint32_t, ApeAnimScriptState*>;

// AVL<float, ApeAnimScriptEvent*> (time-keyed events)
// @func 0x0078F180 AVL<float,ApeAnimScriptEvent*>::RotateRight
// @func 0x0078F2C8 AVL<float,ApeAnimScriptEvent*>::RotateLeft
// @func 0x0078F410 AVL<float,ApeAnimScriptEvent*>::Insert
// @func 0x0078F898 AVL<float,ApeAnimScriptEvent*>::Add
// @func 0x0078FA2C AVL<float,ApeAnimScriptEvent*>::ReplaceWithPredecessor
// @func 0x0078FD34 AVL<float,ApeAnimScriptEvent*>::RemoveFrom
// @func 0x00790920 AVL<float,ApeAnimScriptEvent*>::Search
template class AVL<float, ApeAnimScriptEvent*>;

// AVL<unsigned int, ApeAnimScript*>
// @func 0x00791EF8 AVL<uint,ApeAnimScript*>::RotateRight
// @func 0x00792040 AVL<uint,ApeAnimScript*>::RotateLeft
// @func 0x00792188 AVL<uint,ApeAnimScript*>::ReplaceWithPredecessor
// @func 0x00792490 AVL<uint,ApeAnimScript*>::RemoveFrom
// @func 0x00793080 AVL<uint,ApeAnimScript*>::Search
// @func 0x007930E8 AVL<uint,ApeAnimScript*>::Insert
// @func 0x00793568 AVL<uint,ApeAnimScript*>::Add
template class AVL<uint32_t, ApeAnimScript*>;

// AVL<unsigned int, ApeLevelDefXMLLoader*>
// @func 0x00796A80 AVL<uint,ApeLevelDefXMLLoader*>::RotateRight
// @func 0x00796BC8 AVL<uint,ApeLevelDefXMLLoader*>::RotateLeft
// @func 0x00796D10 AVL<uint,ApeLevelDefXMLLoader*>::ReplaceWithPredecessor
// @func 0x00797018 AVL<uint,ApeLevelDefXMLLoader*>::RemoveFrom
// @func 0x00797C08 AVL<uint,ApeLevelDefXMLLoader*>::Search
// @func 0x00798E60 AVL<uint,ApeLevelDefXMLLoader*>::Insert
// @func 0x007992E0 AVL<uint,ApeLevelDefXMLLoader*>::Add
template class AVL<uint32_t, ApeLevelDefXMLLoader*>;

// AVL<unsigned int, ApeLevelDef*>
// @func 0x00797C70 AVL<uint,ApeLevelDef*>::RotateRight
// @func 0x00797DB8 AVL<uint,ApeLevelDef*>::RotateLeft
// @func 0x00797F00 AVL<uint,ApeLevelDef*>::ReplaceWithPredecessor
// @func 0x00798208 AVL<uint,ApeLevelDef*>::RemoveFrom
// @func 0x00798DF8 AVL<uint,ApeLevelDef*>::Search
// @func 0x007995FC AVL<uint,ApeLevelDef*>::Insert
// @func 0x00799A7C AVL<uint,ApeLevelDef*>::Add
template class AVL<uint32_t, ApeLevelDef*>;

} // namespace ape
