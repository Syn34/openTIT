// openTIT - Compact arrays and sorted key/value maps.
#include "ape/container/ApeTSortedMap.h"

namespace ape {

// Inlined into every map constructor on the console; without an allocator
// the size-bucket pools are used.
ApeCompactArrayBase::ApeCompactArrayBase(uint16_t elemSize, uint16_t capacity, ApeAllocator* allocator)
    : m_capacity(capacity), m_elemSizeAndDirty((uint16_t)(elemSize << 1))
{
    m_allocator = allocator ? allocator : GetBucketAllocator();
    if (m_capacity)
        m_data = m_allocator->Alloc((size_t)m_capacity * elemSize, false, 0x10);
    OnInit();
}

// @func 0x0054EC94 ApeCompactArrayBase::~ApeCompactArrayBase
// The owner releases the storage explicitly (see Release()).
ApeCompactArrayBase::~ApeCompactArrayBase()
{
    Release();
}

// @func 0x00558F38 ApeCompactArrayBase::OnInit
void ApeCompactArrayBase::OnInit()
{
    // Intentionally empty in the shipping build.
}

// @func 0x00558F3C ApeCompactArrayBase::Grow
void ApeCompactArrayBase::Grow(uint32_t minCapacity)
{
    uint32_t capacity = (uint32_t)((m_capacity + 16) & 0xFFFF);
    if (capacity < (minCapacity & 0xFFFF))
        capacity = minCapacity & 0xFFFF;
    const size_t elemSize = ElementSize();
    const size_t oldBytes = (size_t)m_capacity * elemSize;
    void* storage = m_allocator->Alloc(capacity * elemSize, false, 0x10);
    if (m_data) {
        std::memcpy(storage, m_data, oldBytes);
        m_allocator->Free(m_data, oldBytes);
    }
    m_data = storage;
    m_capacity = (uint16_t)capacity;
}

void ApeCompactArrayBase::Release()
{
    if (m_data)
        m_allocator->Free(m_data, (size_t)m_capacity * ElementSize());
    m_data = nullptr;
    m_capacity = 0;
    m_count = 0;
}

void* ApeCompactArrayBase::AppendRaw()
{
    if (m_count == m_capacity)
        Grow((uint32_t)(m_capacity & 0x7FFF) << 1);
    void* slot = static_cast<uint8_t*>(m_data) + (size_t)m_count * ElementSize();
    ++m_count;
    if (m_count > m_peak)
        m_peak = m_count;
    return slot;
}

// The console emitted one private copy of the key comparison per map
// instantiation; all copies are identical.
// @func 0x007365F8 CompareSortedMapKeys (copy 1)
// @func 0x00736628 CompareSortedMapKeys (copy 2)
// @func 0x00736658 CompareSortedMapKeys (copy 3)
// @func 0x00736DD0 CompareSortedMapKeys (copy 4)
// @func 0x00736FC4 CompareSortedMapKeys (copy 5)
// @func 0x00736FF4 CompareSortedMapKeys (copy 6)
// @func 0x00739840 CompareSortedMapKeys (copy 7)
// @func 0x0073990C CompareSortedMapKeys (copy 8)
// @func 0x0073BBF8 CompareSortedMapKeys (copy 9)
// @func 0x0073C9F8 CompareSortedMapKeys (copy 10)
// @func 0x0074307C CompareSortedMapKeys (copy 11)
// @func 0x00746B90 CompareSortedMapKeys (copy 12)
// @func 0x00746CF8 CompareSortedMapKeys (copy 13)
// @func 0x00746D28 CompareSortedMapKeys (copy 14)
// @func 0x00746D58 CompareSortedMapKeys (copy 15)
// @func 0x0074EA54 CompareSortedMapKeys (copy 16)
// @func 0x0074EA84 CompareSortedMapKeys (copy 17)
// @func 0x0074ECCC CompareSortedMapKeys (copy 18)
// @func 0x0075396C CompareSortedMapKeys (copy 19)
// @func 0x00759E44 CompareSortedMapKeys (copy 20)
// @func 0x0075D5C4 CompareSortedMapKeys (copy 21)
// @func 0x0075D5F4 CompareSortedMapKeys (copy 22)
// @func 0x00761E98 CompareSortedMapKeys (copy 23)
// @func 0x00762158 CompareSortedMapKeys (copy 24)
// @func 0x00762260 CompareSortedMapKeys (copy 25)
// @func 0x00762290 CompareSortedMapKeys (copy 26)
// @func 0x007622C0 CompareSortedMapKeys (copy 27)
// @func 0x007623C8 CompareSortedMapKeys (copy 28)
// @func 0x00762410 CompareSortedMapKeys (copy 29)
// @func 0x00762518 CompareSortedMapKeys (copy 30)
// @func 0x00762620 CompareSortedMapKeys (copy 31)
// @func 0x00762728 CompareSortedMapKeys (copy 32)
// @func 0x00762830 CompareSortedMapKeys (copy 33)
// @func 0x00762938 CompareSortedMapKeys (copy 34)
// @func 0x0076308C CompareSortedMapKeys (copy 35)
// @func 0x00764C90 CompareSortedMapKeys (copy 36)
// @func 0x00766A4C CompareSortedMapKeys (copy 37)
// @func 0x00766A7C CompareSortedMapKeys (copy 38)
// @func 0x00766AAC CompareSortedMapKeys (copy 39)
// @func 0x00766ADC CompareSortedMapKeys (copy 40)
// @func 0x00766B0C CompareSortedMapKeys (copy 41)
// @func 0x00766B3C CompareSortedMapKeys (copy 42)
// @func 0x007670DC CompareSortedMapKeys (copy 43)
// @func 0x0076710C CompareSortedMapKeys (copy 44)
// @func 0x0076713C CompareSortedMapKeys (copy 45)
// @func 0x00768DD0 CompareSortedMapKeys (copy 46)
// @func 0x00768E00 CompareSortedMapKeys (copy 47)
// @func 0x0076BA58 CompareSortedMapKeys (copy 48)
// @func 0x0076BA88 CompareSortedMapKeys (copy 49)
// @func 0x007707F0 CompareSortedMapKeys (copy 50)
// @func 0x00770820 CompareSortedMapKeys (copy 51)
// @func 0x00770850 CompareSortedMapKeys (copy 52)
// @func 0x00770880 CompareSortedMapKeys (copy 53)
// @func 0x007708B0 CompareSortedMapKeys (copy 54)
// @func 0x007708E0 CompareSortedMapKeys (copy 55)
// @func 0x0077288C CompareSortedMapKeys (copy 56)
// @func 0x007728BC CompareSortedMapKeys (copy 57)
// @func 0x0077A97C CompareSortedMapKeys (copy 58)
// @func 0x0077EB6C CompareSortedMapKeys (copy 59)
// @func 0x00784A74 CompareSortedMapKeys (copy 60)
// @func 0x00784AA4 CompareSortedMapKeys (copy 61)
// @func 0x00784AD4 CompareSortedMapKeys (copy 62)
// @func 0x00784B04 CompareSortedMapKeys (copy 63)
// @func 0x00784B34 CompareSortedMapKeys (copy 64)
// @func 0x00784B64 CompareSortedMapKeys (copy 65)
// @func 0x00784B94 CompareSortedMapKeys (copy 66)
// @func 0x00788384 CompareSortedMapKeys (copy 67)
// @func 0x0078848C CompareSortedMapKeys (copy 68)
// @func 0x00788EC4 CompareSortedMapKeys (copy 69)
// @func 0x00789F68 CompareSortedMapKeys (copy 70)
// @func 0x00789F98 CompareSortedMapKeys (copy 71)
// @func 0x00789FC8 CompareSortedMapKeys (copy 72)
// @func 0x00789FF8 CompareSortedMapKeys (copy 73)
// @func 0x00794780 CompareSortedMapKeys (copy 74)
// @func 0x007947B0 CompareSortedMapKeys (copy 75)
// @func 0x007947E0 CompareSortedMapKeys (copy 76)
// @func 0x00795874 CompareSortedMapKeys (copy 77)
// @func 0x0079A008 CompareSortedMapKeys (copy 78)
// @func 0x0079B8BC CompareSortedMapKeys (copy 79)
// @func 0x0079B8EC CompareSortedMapKeys (copy 80)
// @func 0x0079B91C CompareSortedMapKeys (copy 81)
// @func 0x007A2218 CompareSortedMapKeys (copy 82)
// @func 0x007A2248 CompareSortedMapKeys (copy 83)
// @func 0x007A38FC CompareSortedMapKeys (copy 84)
// @func 0x007A392C CompareSortedMapKeys (copy 85)
// @func 0x007A4D68 CompareSortedMapKeys (copy 86)
// @func 0x007A4E50 CompareSortedMapKeys (copy 87)
// @func 0x007AA1F4 CompareSortedMapKeys (copy 88)
// @func 0x007AA3B4 CompareSortedMapKeys (copy 89)
// @func 0x007AA574 CompareSortedMapKeys (copy 90)
// @func 0x007AA66C CompareSortedMapKeys (copy 91)
int CompareSortedMapKeys(const void* a, const void* b)
{
    const uint32_t ka = static_cast<const ApeSortedMapKey*>(a)->key;
    const uint32_t kb = static_cast<const ApeSortedMapKey*>(b)->key;
    return ka > kb ? 1 : (ka < kb ? -1 : 0);
}

// Every 4-byte-value instantiation of Find/Remove compiles to the same code
// (only the address of its private comparator differs), so one
// instantiation covers all the copies below.
// @func 0x00732F38 ApeTSortedMap<V>::Find (copy 1)
// @func 0x007332B4 ApeTSortedMap<V>::Find (copy 2)
// @func 0x0073343C ApeTSortedMap<V>::Find (copy 3)
// @func 0x007335C4 ApeTSortedMap<V>::Find (copy 4)
// @func 0x0073374C ApeTSortedMap<V>::Find (copy 5)
// @func 0x00733F8C ApeTSortedMap<V>::Find (copy 6)
// @func 0x00734750 ApeTSortedMap<V>::Find (copy 7)
// @func 0x007399A4 ApeTSortedMap<V>::Find (copy 8)
// @func 0x0073B608 ApeTSortedMap<V>::Find (copy 9)
// @func 0x00742EF4 ApeTSortedMap<V>::Find (copy 10)
// @func 0x0074493C ApeTSortedMap<V>::Find (copy 11)
// @func 0x00744AC4 ApeTSortedMap<V>::Find (copy 12)
// @func 0x00744C4C ApeTSortedMap<V>::Find (copy 13)
// @func 0x00744DD4 ApeTSortedMap<V>::Find (copy 14)
// @func 0x00744F5C ApeTSortedMap<V>::Find (copy 15)
// @func 0x00749A14 ApeTSortedMap<V>::Find (copy 16)
// @func 0x00749B9C ApeTSortedMap<V>::Find (copy 17)
// @func 0x00759654 ApeTSortedMap<V>::Find (copy 18)
// @func 0x0075C054 ApeTSortedMap<V>::Find (copy 19)
// @func 0x0075C1DC ApeTSortedMap<V>::Find (copy 20)
// @func 0x0075E260 ApeTSortedMap<V>::Find (copy 21)
// @func 0x0075E3E8 ApeTSortedMap<V>::Find (copy 22)
// @func 0x0075E570 ApeTSortedMap<V>::Find (copy 23)
// @func 0x0075E8EC ApeTSortedMap<V>::Find (copy 24)
// @func 0x0075EA74 ApeTSortedMap<V>::Find (copy 25)
// @func 0x0075F004 ApeTSortedMap<V>::Find (copy 26)
// @func 0x0075F380 ApeTSortedMap<V>::Find (copy 27)
// @func 0x0075F6FC ApeTSortedMap<V>::Find (copy 28)
// @func 0x0075FA78 ApeTSortedMap<V>::Find (copy 29)
// @func 0x0075FC00 ApeTSortedMap<V>::Find (copy 30)
// @func 0x0075FD88 ApeTSortedMap<V>::Find (copy 31)
// @func 0x00760538 ApeTSortedMap<V>::Find (copy 32)
// @func 0x0076116C ApeTSortedMap<V>::Find (copy 33)
// @func 0x0076611C ApeTSortedMap<V>::Find (copy 34)
// @func 0x007662A4 ApeTSortedMap<V>::Find (copy 35)
// @func 0x0076642C ApeTSortedMap<V>::Find (copy 36)
// @func 0x007665B4 ApeTSortedMap<V>::Find (copy 37)
// @func 0x0076673C ApeTSortedMap<V>::Find (copy 38)
// @func 0x007668C4 ApeTSortedMap<V>::Find (copy 39)
// @func 0x00766C44 ApeTSortedMap<V>::Find (copy 40)
// @func 0x00766DCC ApeTSortedMap<V>::Find (copy 41)
// @func 0x00766F54 ApeTSortedMap<V>::Find (copy 42)
// @func 0x00768AC0 ApeTSortedMap<V>::Find (copy 43)
// @func 0x00768C48 ApeTSortedMap<V>::Find (copy 44)
// @func 0x0076AA2C ApeTSortedMap<V>::Find (copy 45)
// @func 0x0076ABB4 ApeTSortedMap<V>::Find (copy 46)
// @func 0x0076D894 ApeTSortedMap<V>::Find (copy 47)
// @func 0x0076DC10 ApeTSortedMap<V>::Find (copy 48)
// @func 0x0076DD98 ApeTSortedMap<V>::Find (copy 49)
// @func 0x0076DF20 ApeTSortedMap<V>::Find (copy 50)
// @func 0x0076E0A8 ApeTSortedMap<V>::Find (copy 51)
// @func 0x0076E230 ApeTSortedMap<V>::Find (copy 52)
// @func 0x00771FC4 ApeTSortedMap<V>::Find (copy 53)
// @func 0x00774154 ApeTSortedMap<V>::Find (copy 54)
// @func 0x0077B1FC ApeTSortedMap<V>::Find (copy 55)
// @func 0x00783E80 ApeTSortedMap<V>::Find (copy 56)
// @func 0x00784308 ApeTSortedMap<V>::Find (copy 57)
// @func 0x007845DC ApeTSortedMap<V>::Find (copy 58)
// @func 0x00784764 ApeTSortedMap<V>::Find (copy 59)
// @func 0x007848EC ApeTSortedMap<V>::Find (copy 60)
// @func 0x00786C0C ApeTSortedMap<V>::Find (copy 61)
// @func 0x00788D3C ApeTSortedMap<V>::Find (copy 62)
// @func 0x007890F8 ApeTSortedMap<V>::Find (copy 63)
// @func 0x007936FC ApeTSortedMap<V>::Find (copy 64)
// @func 0x00793884 ApeTSortedMap<V>::Find (copy 65)
// @func 0x00793A0C ApeTSortedMap<V>::Find (copy 66)
// @func 0x007948E4 ApeTSortedMap<V>::Find (copy 67)
// @func 0x00799474 ApeTSortedMap<V>::Find (copy 68)
// @func 0x0079A9F8 ApeTSortedMap<V>::Find (copy 69)
// @func 0x0079AB80 ApeTSortedMap<V>::Find (copy 70)
// @func 0x0079C8C4 ApeTSortedMap<V>::Find (copy 71)
// @func 0x0079DD10 ApeTSortedMap<V>::Find (copy 72)
// @func 0x007A2D40 ApeTSortedMap<V>::Find (copy 73)
// @func 0x007A3014 ApeTSortedMap<V>::Find (copy 74)
// @func 0x007A46EC ApeTSortedMap<V>::Find (copy 75)
// @func 0x007A68DC ApeTSortedMap<V>::Find (copy 76)
// @func 0x007A6A64 ApeTSortedMap<V>::Find (copy 77)
// @func 0x007A6BEC ApeTSortedMap<V>::Find (copy 78)
// @func 0x007A6D74 ApeTSortedMap<V>::Find (copy 79)
// @func 0x007504F0 ApeTSortedMap<V>::Find (copy 80)
// @func 0x007330C0 ApeTSortedMap<V>::Remove (copy 1)
// @func 0x00744748 ApeTSortedMap<V>::Remove (copy 2)
// @func 0x007597DC ApeTSortedMap<V>::Remove (copy 3)
// @func 0x0075DC84 ApeTSortedMap<V>::Remove (copy 4)
// @func 0x0075DE78 ApeTSortedMap<V>::Remove (copy 5)
// @func 0x0075E06C ApeTSortedMap<V>::Remove (copy 6)
// @func 0x0075E6F8 ApeTSortedMap<V>::Remove (copy 7)
// @func 0x0075EE10 ApeTSortedMap<V>::Remove (copy 8)
// @func 0x0075F18C ApeTSortedMap<V>::Remove (copy 9)
// @func 0x0075F508 ApeTSortedMap<V>::Remove (copy 10)
// @func 0x0075F884 ApeTSortedMap<V>::Remove (copy 11)
// @func 0x0076D6A0 ApeTSortedMap<V>::Remove (copy 12)
// @func 0x0076DA1C ApeTSortedMap<V>::Remove (copy 13)
// @func 0x00771DD0 ApeTSortedMap<V>::Remove (copy 14)
// @func 0x00783C8C ApeTSortedMap<V>::Remove (copy 15)
// @func 0x00786A18 ApeTSortedMap<V>::Remove (copy 16)
// @func 0x00788B48 ApeTSortedMap<V>::Remove (copy 17)
// @func 0x0079AD08 ApeTSortedMap<V>::Remove (copy 18)
// @func 0x0079AEFC ApeTSortedMap<V>::Remove (copy 19)
// @func 0x0079E4C8 ApeTSortedMap<V>::Remove (copy 20)
// @func 0x0079E7B0 ApeTSortedMap<V>::Remove (copy 21)
// @func 0x007A44F8 ApeTSortedMap<V>::Remove (copy 22)
// @func 0x007A7A80 ApeTSortedMap<V>::Remove (copy 23)
// @func 0x007A7C74 ApeTSortedMap<V>::Remove (copy 24)
// @func 0x007A7E68 ApeTSortedMap<V>::Remove (copy 25)
// @func 0x007A805C ApeTSortedMap<V>::Remove (copy 26)
// @func 0x007A8250 ApeTSortedMap<V>::Remove (copy 27)
// @func 0x007A8544 ApeTSortedMap<V>::Remove (copy 28)
// @func 0x007A8738 ApeTSortedMap<V>::Remove (copy 29)
// @func 0x007A892C ApeTSortedMap<V>::Remove (copy 30)
// @func 0x007A8B20 ApeTSortedMap<V>::Remove (copy 31)
// @func 0x007A8D40 ApeTSortedMap<V>::Remove (copy 32)
// @func 0x007A8F34 ApeTSortedMap<V>::Remove (copy 33)
// @func 0x007A92F4 ApeTSortedMap<V>::Remove (copy 34)
// @func 0x00786D94 ApeTSortedMap<V>::Remove (copy 35)
template class ApeTSortedMap<uint32_t>;

// The float-valued maps (the bone weight lists of the animation layers)
// compiled Find/Remove with a slightly different schedule.
// @func 0x0076ECC4 ApeTSortedMap<V>::Find (float values)
// @func 0x0077016C ApeTSortedMap<V>::Remove (float values)
template class ApeTSortedMap<float>;

// The constructor with an explicit comparator was emitted out of line for
// fourteen 4-byte-value maps (identical code).
// @func 0x00761DC0 ApeTSortedMap<V>::ApeTSortedMap (copy 1)
// @func 0x00761EC8 ApeTSortedMap<V>::ApeTSortedMap (copy 2)
// @func 0x00761FA0 ApeTSortedMap<V>::ApeTSortedMap (copy 3)
// @func 0x00762078 ApeTSortedMap<V>::ApeTSortedMap (copy 4)
// @func 0x00762188 ApeTSortedMap<V>::ApeTSortedMap (copy 5)
// @func 0x007622F0 ApeTSortedMap<V>::ApeTSortedMap (copy 6)
// @func 0x00762440 ApeTSortedMap<V>::ApeTSortedMap (copy 7)
// @func 0x00762548 ApeTSortedMap<V>::ApeTSortedMap (copy 8)
// @func 0x00762650 ApeTSortedMap<V>::ApeTSortedMap (copy 9)
// @func 0x00762758 ApeTSortedMap<V>::ApeTSortedMap (copy 10)
// @func 0x00762860 ApeTSortedMap<V>::ApeTSortedMap (copy 11)
// @func 0x00762968 ApeTSortedMap<V>::ApeTSortedMap (copy 12)
// @func 0x007883B4 ApeTSortedMap<V>::ApeTSortedMap (copy 13)
// @func 0x007884BC ApeTSortedMap<V>::ApeTSortedMap (copy 14)
//
// Sorted insertion, 4-byte values (copies 1-4) and an 8-byte value (copy 5).
// @func 0x00759508 ApeTSortedMap<V>::Insert (copy 1)
// @func 0x00784490 ApeTSortedMap<V>::Insert (copy 2)
// @func 0x007A2BF4 ApeTSortedMap<V>::Insert (copy 3)
// @func 0x007A2EC8 ApeTSortedMap<V>::Insert (copy 4)
// @func 0x00784194 ApeTSortedMap<V>::Insert (copy 5, 8-byte value)
//
// Index lookup for 8-, 24- and 16-byte values.
// @func 0x00784008 ApeTSortedMap<V>::FindIndex (copy 1, 8-byte value)
// @func 0x0078987C ApeTSortedMap<V>::FindIndex (copy 2, 24-byte value)
// @func 0x00789A1C ApeTSortedMap<V>::FindIndex (copy 3, 16-byte value)
//
// The tags above sit on the template members in ApeTSortedMap.h; this
// explicit instantiation makes sure every one of them is compiled.
struct ApeSortedMapValue8 { uint32_t a, b; };
template class ApeTSortedMap<ApeSortedMapValue8>;

} // namespace ape
