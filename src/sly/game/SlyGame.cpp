// openTIT - Game world object: the parts owned by the game shell.
#include "sly/game/SlyGame.h"

namespace sly {

SlyGame* g_game = nullptr;

// @func 0x00458B50 SlyGame::ResetFrameLists
void SlyGame::ResetFrameLists()
{
    for (int i = 0; i < kFrameListCount; ++i)
        callbackLists[kFirstFrameList + i].Clear();
}

// @func 0x0046B5F8 SlyGame::AddCallback
void SlyGame::AddCallback(int list, void* function, void* context, entity::Entity* owner)
{
    WorldCallback entry;
    entry.function = function;
    entry.context = context;
    entry.owner = owner;
    callbackLists[list].PushBack(entry);
}

// @func 0x0046B7A4 SlyGame::RemoveCallback
void SlyGame::RemoveCallback(int list, void* context, entity::Entity* owner)
{
    ape::ApeTArray<WorldCallback>& callbacks = callbackLists[list];
    for (size_t i = 0; i < callbacks.Count();) {
        if (callbacks[i].owner == owner && callbacks[i].context == context)
            callbacks.RemoveAtSwap(i);   // re-test the entry moved into slot i
        else
            ++i;
    }
}

// @func 0x0045900C SlyGame::RunCallbacks
void SlyGame::RunCallbacks(int list, float dt, bool clear)
{
    // NOTE: the console passes the frame time in a float register next to
    // the context; the port's callbacks take (unused, context) only.
    (void)dt;
    using Fn = void (*)(void* unused, void* context);
    ape::ApeTArray<WorldCallback>& callbacks = callbackLists[list];
    for (size_t i = 0; i < callbacks.Count(); ++i) {
        const WorldCallback& entry = callbacks[i];
        reinterpret_cast<Fn>(entry.function)(nullptr, entry.context);
    }
    if (clear)
        callbacks.Clear();
}

// @func 0x0046B6CC SlyGame::ForgetEntity
void SlyGame::ForgetEntity(entity::Entity* owner)
{
    for (ape::ApeTArray<WorldCallback>& callbacks : callbackLists) {
        for (size_t i = 0; i < callbacks.Count();) {
            if (callbacks[i].owner == owner)
                callbacks.RemoveAtSwap(i);
            else
                ++i;
        }
    }
}

} // namespace sly
