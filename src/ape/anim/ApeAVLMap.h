// openTIT - AVLMap: an AVL tree bundled with a traversal cursor.
//
// Every name-keyed registry of the anim script system (variables, events,
// states, scripts, libraries, the global prototype tables) is an AVL tree
// followed in memory by a cursor: a pointer back to the tree, a stack of the
// nodes visited on the way down and the current node. Find() leaves the
// cursor on the matching node, First()/Next() walk the tree in key order.
// The stack holds 20 entries, which bounds the tree height the console code
// could walk; the balanced trees used here never get close.
#pragma once

#include "ape/container/ApeAVL.h"

#include <cstdint>

namespace ape {

template <class KeyType, class ItemType>
class AVLMap {
public:
    using Tree = AVL<KeyType, ItemType>;
    using Node = typename Tree::Node;

    AVLMap() = default;
    AVLMap(const AVLMap&) = delete;
    AVLMap& operator=(const AVLMap&) = delete;

    uint32_t Count() const { return m_tree.Count(); }
    bool IsEmpty() const { return m_tree.Root() == nullptr; }

    // Inserts (key, item). Callers check for an existing key first.
    void Add(KeyType key, ItemType item) { m_tree.Add(key, item); }
    // Removes the key; the cursor is reset because its stack may be stale.
    bool Remove(KeyType key)
    {
        ResetCursor();
        return m_tree.Remove(key);
    }
    void Clear()
    {
        ResetCursor();
        m_tree.Clear();
    }

    // Searches for `key`. On success the cursor sits on the node and its item
    // is returned; otherwise the cursor stays on the last node visited (the
    // place the key would be inserted below) and ItemType() is returned.
    ItemType Find(KeyType key)
    {
        m_depth = 0;
        Node* node = m_tree.Root();
        m_current = node;
        while (node) {
            if (node->key == key) {
                m_current = node;
                return node->item;
            }
            Node* next = (key < node->key) ? node->left : node->right;
            if (!next) {
                m_current = node;
                return ItemType();
            }
            Push(node);
            node = next;
        }
        return ItemType();
    }
    bool Contains(KeyType key)
    {
        Find(key);
        return m_current && m_current->key == key;
    }

    // Positions the cursor on the first node whose key is >= `key`.
    ItemType SeekLowerBound(KeyType key)
    {
        Find(key);
        if (m_current && m_current->key < key)
            Next();
        return Current();
    }

    // In-order traversal. Next() returns ItemType() once the walk is over.
    ItemType First()
    {
        m_depth = 0;
        m_current = m_tree.Root();
        DescendLeft();
        return Current();
    }
    ItemType Next()
    {
        Node* node = m_current;
        if (!node)
            return ItemType();
        if (node->right) {
            Push(node);
            m_current = node->right;
            DescendLeft();
            return Current();
        }
        // Climb until we arrive from a left child.
        while (m_depth > 0) {
            Node* parent = m_stack[--m_depth];
            if (parent->left == node) {
                m_current = parent;
                return Current();
            }
            node = parent;
        }
        m_current = nullptr;
        return ItemType();
    }
    bool AtEnd() const { return m_current == nullptr; }
    ItemType Current() const { return m_current ? m_current->item : ItemType(); }
    KeyType CurrentKey() const { return m_current ? m_current->key : KeyType(); }

    // Calls fn(item) for every item in key order. The callback must not add
    // or remove entries of this map.
    template <class Fn>
    void ForEach(Fn&& fn)
    {
        for (First(); !AtEnd(); Next())
            fn(m_current->item);
    }

private:
    static constexpr int kStackSize = 20;

    void Push(Node* node)
    {
        if (m_depth < kStackSize)
            m_stack[m_depth++] = node;
    }
    void DescendLeft()
    {
        while (m_current && m_current->left) {
            Push(m_current);
            m_current = m_current->left;
        }
    }
    void ResetCursor()
    {
        m_depth = 0;
        m_current = nullptr;
    }

    Tree m_tree;
    Node* m_stack[kStackSize] = {};
    int16_t m_depth = 0;
    Node* m_current = nullptr;
};

} // namespace ape
