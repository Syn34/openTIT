// openTIT - AVL<KeyType, ItemType>: balanced binary search tree keyed map.
//
// The engine uses it for name-hash keyed registries (anim scripts, level
// definitions, physics bodies, ...). Nodes come from one shared free list
// per instantiation. Balancing can be switched off per tree; duplicate keys
// are ignored on insert.
#pragma once

#include "ape/memory/ApeTFreeList.h"

#include <cstdint>

namespace ape {

template <class KeyType, class ItemType>
class AVL {
public:
    template <class K, class I>
    struct AVLNode {
        int16_t balance;   // height(right) - height(left)
        int16_t height;    // 1 for a leaf
        K key;
        I item;
        AVLNode* left;
        AVLNode* right;
    };
    using Node = AVLNode<KeyType, ItemType>;

    explicit AVL(bool autoBalance = true) : m_balance(autoBalance) {}
    ~AVL() { Clear(); }
    AVL(const AVL&) = delete;
    AVL& operator=(const AVL&) = delete;

    // Inserts (key, item). The element count is bumped even for a duplicate key.
    void Add(KeyType key, ItemType item);
    bool Find(KeyType key, ItemType& outItem) const { return Search(key, outItem, m_root); }
    // Searches the subtree rooted at `node`.
    bool Search(KeyType key, ItemType& outItem, const Node* node) const;
    // Removes `key`; returns true if it was present.
    bool Remove(KeyType key);
    void Clear();

    uint32_t Count() const { return m_count; }
    Node* Root() const { return m_root; }

    static ApeTFreeList<Node>& GetAllocator();

private:
    Node* NewNode(KeyType key, ItemType item);
    void Insert(KeyType key, ItemType item, Node*& slot);
    void RemoveFrom(Node*& slot, KeyType key, bool& removed);
    void ReplaceWithPredecessor(Node*& target, Node*& slot, bool& removed);
    void RotateRight(Node*& slot);
    void RotateLeft(Node*& slot);
    void Rebalance(Node*& slot);
    static void UpdateHeight(Node* node);
    static void FreeSubtree(Node* node);

    Node* m_root = nullptr;
    uint32_t m_count = 0;
    bool m_balance;
};

// ---------------------------------------------------------------------------

template <class K, class I>
ApeTFreeList<typename AVL<K, I>::Node>& AVL<K, I>::GetAllocator()
{
    static ApeTFreeList<Node>* s_allocator = nullptr;
    if (!s_allocator) {
        s_allocator = new ApeTFreeList<Node>();
        s_allocator->SetName("static ApeTFreeList<AVL<KeyType, ItemType>::AVLNode<KeyType, ItemType>> "
                             "&AVL<KeyType, ItemType>::GetAllocator()");
        s_allocator->Init(0x40, nullptr, false);
    }
    return *s_allocator;
}

template <class K, class I>
typename AVL<K, I>::Node* AVL<K, I>::NewNode(K key, I item)
{
    Node* node = GetAllocator().AllocT();
    if (node) {
        node->balance = 0;
        node->height = 1;
        node->key = key;
        node->item = item;
        node->left = nullptr;
        node->right = nullptr;
    }
    return node;
}

template <class K, class I>
void AVL<K, I>::UpdateHeight(Node* node)
{
    if (!node)
        return;
    const int16_t hl = node->left ? node->left->height : 0;
    const int16_t hr = node->right ? node->right->height : 0;
    node->height = (int16_t)((hl > hr ? hl : hr) + 1);
    node->balance = (int16_t)(hr - hl);
}

// Restores the AVL property at `slot` after one of its subtrees changed height.
template <class K, class I>
void AVL<K, I>::Rebalance(Node*& slot)
{
    UpdateHeight(slot);
    Node* node = slot;
    if (!node)
        return;
    if (node->balance > 1 && node->right) {
        if (node->right->balance < 0)
            RotateRight(node->right);   // right-left case
        RotateLeft(slot);
    }
    node = slot;
    if (node && node->balance < -1 && node->left) {
        if (node->left->balance > 0)
            RotateLeft(node->left);     // left-right case
        RotateRight(slot);
    }
}

template <class K, class I>
void AVL<K, I>::RotateRight(Node*& slot)
{
    Node* top = slot;
    Node* pivot = top->left;
    slot = pivot;
    if (!pivot)
        return;
    top->left = pivot->right;
    pivot->right = top;
    UpdateHeight(pivot->left);
    UpdateHeight(pivot->right);
    UpdateHeight(pivot);
}

template <class K, class I>
void AVL<K, I>::RotateLeft(Node*& slot)
{
    Node* top = slot;
    Node* pivot = top->right;
    slot = pivot;
    if (!pivot)
        return;
    top->right = pivot->left;
    pivot->left = top;
    UpdateHeight(pivot->left);
    UpdateHeight(pivot->right);
    UpdateHeight(pivot);
}

template <class K, class I>
void AVL<K, I>::Insert(K key, I item, Node*& slot)
{
    Node* node = slot;
    if (key < node->key) {
        if (!node->left)
            node->left = NewNode(key, item);
        else
            Insert(key, item, node->left);
    } else if (node->key < key) {
        if (!node->right)
            node->right = NewNode(key, item);
        else
            Insert(key, item, node->right);
    }
    if (m_balance)
        Rebalance(slot);
}

template <class K, class I>
void AVL<K, I>::Add(K key, I item)
{
    ++m_count;
    if (!m_root)
        m_root = NewNode(key, item);
    else
        Insert(key, item, m_root);
}

template <class K, class I>
bool AVL<K, I>::Search(K key, I& outItem, const Node* node) const
{
    while (node) {
        if (node->key == key) {
            outItem = node->item;
            return true;
        }
        node = (key < node->key) ? node->left : node->right;
    }
    return false;
}

// Unlinks the right-most node below `slot` (the in-order predecessor of
// `target`) and moves its key/item into `target`.
template <class K, class I>
void AVL<K, I>::ReplaceWithPredecessor(Node*& target, Node*& slot, bool& removed)
{
    Node* node = slot;
    if (!node->right) {
        target->key = node->key;
        target->item = node->item;
        slot = node->left;
        GetAllocator().Free(node);
        removed = true;
        return;
    }
    ReplaceWithPredecessor(target, node->right, removed);
    if (removed && m_balance)
        Rebalance(slot);
}

template <class K, class I>
void AVL<K, I>::RemoveFrom(Node*& slot, K key, bool& removed)
{
    Node* node = slot;
    if (!node) {
        removed = false;
        return;
    }
    if (key < node->key) {
        RemoveFrom(node->left, key, removed);
        if (removed && m_balance)
            Rebalance(slot);
    } else if (node->key < key) {
        RemoveFrom(node->right, key, removed);
        if (removed && m_balance)
            Rebalance(slot);
    } else if (!node->right) {
        removed = true;
        slot = node->left;
        GetAllocator().Free(node);
        Rebalance(slot);
    } else if (!node->left) {
        removed = true;
        slot = node->right;
        GetAllocator().Free(node);
        Rebalance(slot);
    } else {
        // Two children: take over the predecessor's contents instead.
        ReplaceWithPredecessor(slot, node->left, removed);
        if (removed && m_balance) {
            Rebalance(slot->left);
            Rebalance(slot);
        }
    }
}

template <class K, class I>
bool AVL<K, I>::Remove(K key)
{
    bool removed = false;
    RemoveFrom(m_root, key, removed);
    if (removed && m_count)
        --m_count;   // NOTE: the console callers adjust the count themselves
    return removed;
}

template <class K, class I>
void AVL<K, I>::FreeSubtree(Node* node)
{
    if (!node)
        return;
    FreeSubtree(node->left);
    FreeSubtree(node->right);
    GetAllocator().Free(node);
}

template <class K, class I>
void AVL<K, I>::Clear()
{
    FreeSubtree(m_root);
    m_root = nullptr;
    m_count = 0;
}

} // namespace ape
