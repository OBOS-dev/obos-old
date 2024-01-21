/*
	oboskrnl/vector.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <pair.h>

#include <memory_manipulation.h>

#include <allocators/liballoc.h>

namespace obos
{
    namespace utils
    {
        template<typename Key>
        inline size_t defaultHasher(const Key& key)
        {
            const byte* _key = (byte*)&key;

            size_t h = 0, g = 0;

			for (size_t i = 0; i < sizeof(Key); i++)
			{
				h = (h << 4) + *_key++;
				if ((g = h & 0xf0000000))
					h ^= g >> 24;
				h &= ~g;
			}
			return h;
        }
        template<>
        inline size_t defaultHasher<const char*>(const char* const& key)
        {
            const byte* _key = (byte*)key;

            size_t h = 0, g = 0;

            for (size_t i = 0; i < utils::strlen(key); i++)
            {
                h = (h << 4) + *_key++;
                if ((g = h & 0xf0000000))
                    h ^= g >> 24;
                h &= ~g;
            }
            return h;
        }
        template<typename Key>
        inline bool defaultEquals(const Key& key1, const Key& key2)
        {
            return key1 == key2;
        }
        template<typename Key, typename ValT, bool(*equals)(const Key& key1, const Key& key2) = defaultEquals<Key>, size_t(*hash)(const Key& key) = defaultHasher<Key>>
        class Hashmap
        {
        public:
            template<typename _Key, typename _ValT, bool(*_equals)(const Key& key1, const Key& key2) = defaultEquals<Key>, size_t(*_hash)(const Key& key) = defaultHasher<Key>>
            class HashmapIter
            {
            public:
                using _node = Hashmap<_Key, _ValT, _equals, _hash>::listNode;

                HashmapIter() = delete;

                auto& operator*()
				{
                    node* n = (node*)m_currentNode->data;
                    return *n;
				}
				size_t operator++()
				{
                    auto newNode = m_currentNode = m_currentNode->next;
					return ((uintptr_t)(newNode) - (uintptr_t)m_map->m_nodeTable) / sizeof(*m_currentNode);
				}
				size_t operator++(int)
				{
                    auto oldNode = m_currentNode;
                    m_currentNode = m_currentNode->next;
                    return ((uintptr_t)(oldNode) - (uintptr_t)m_map->m_nodeTable) / sizeof(*m_currentNode);
				}
				size_t operator--()
				{
					auto oldNode = m_currentNode;
					m_currentNode = m_currentNode->prev;
                    return ((uintptr_t)(oldNode) - (uintptr_t)m_map->m_nodeTable) / sizeof(*m_currentNode);
				}
				size_t operator--(int)
				{
					auto oldNode = m_currentNode;
                    auto newNode = m_currentNode = m_currentNode->prev;
                    return ((uintptr_t)(oldNode) - (uintptr_t)m_map->m_nodeTable) / sizeof(*m_currentNode);
				}

				operator bool() 
                { 
                    return m_currentNode != nullptr;
                }
            
                friend class Hashmap;
            private:
                HashmapIter(Hashmap<_Key, _ValT, _equals, _hash>* map, _node* currentNode)
                {
                    m_map = map;
                    m_currentNode = currentNode;
                }
                Hashmap<_Key, _ValT, _equals, _hash>* m_map;
                _node* m_currentNode;
            };
            Hashmap()
            {
                m_buckets = (bucket*)kcalloc(m_tableCapacity, sizeof(bucket));
                m_nodeTable = (node*)kcalloc(m_tableCapacity, sizeof(node));
            }

            Hashmap(const Hashmap& other) = delete;
            Hashmap& operator=(const Hashmap& other) = delete;
            Hashmap(Hashmap&& other) = delete;
            Hashmap& operator=(Hashmap&& other) = delete;

            ValT& at(const Key& key) const
            {
                if (!m_tableCapacity)
                    return *(ValT*)nullptr;
                size_t _hash = hash(key) % m_tableCapacity;
                bucket* currentBucket = m_buckets + _hash;
                if (!currentBucket->nChains)
                    return *(ValT*)nullptr;
                chain* currentChain = currentBucket->firstChain;
                node* currentNode = m_nodeTable + currentChain->tableIndex;
                while (!equals(key, *currentNode->key) && currentChain)
                {
                    currentChain = currentChain->next;
                    if (currentChain)
                        currentNode = m_nodeTable + currentChain->tableIndex;
                }
                if (!currentBucket)
                    return *(ValT*)nullptr;
                if (!equals(key, *currentNode->key))
                    return *(ValT*)nullptr;
                return *currentNode->value;
            }

            void emplace_at(const Key& key, const ValT& value)
            {
                // The buckets and chains contain an index to key.
                size_t ourIndex = 0;
                for (size_t i = 0; i < m_tableCapacity; i++)
                {
                    if (!m_nodeTable[i].used)
                    {
                        ourIndex = i;
                        break;
                    }
                }
                if (!m_tableCapacity)
                    expand(m_tableCapacity = 64);
                if (m_nodeTable[ourIndex].used)
                {
                    expand(m_tableCapacity *= 2);
                    ourIndex++;
                }
                size_t _hash = hash(key) % m_tableCapacity;
                bucket* ourBucket = &m_buckets[_hash];
                chain* ourChain = (chain*)kcalloc(1, sizeof(chain));
                ourChain->tableIndex = ourIndex;
                if (ourBucket->lastChain)
                    ourBucket->lastChain->next = ourChain;
                if(!ourBucket->firstChain)
                    ourBucket->firstChain = ourChain;
                ourChain->prev = ourBucket->lastChain;
                ourBucket->lastChain = ourChain;
                ourBucket->nChains++;
                m_nodeTable[ourIndex].key   = new Key{ key };
                m_nodeTable[ourIndex].value = new ValT{ value };
                m_nodeTable[ourIndex].used = true;
                listNode* node = new listNode;
                node->data = m_nodeTable + ourIndex;
                if (m_tail)
                    m_tail->next = node;
                if(!m_head)
                    m_head = node;
                node->prev = m_tail;
                m_tail = node;
                m_nNodes++;
                m_nodeTable[ourIndex].referencingNode = node;
            }

            void remove(const Key& key)
            {
                if (!m_tableCapacity)
                    return;
                size_t _hash = hash(key) % m_tableCapacity;
                bucket* currentBucket = m_buckets + _hash;
                if (!currentBucket->nChains)
                    return;
                chain* currentChain = currentBucket->firstChain;
                node* currentNode = m_nodeTable + currentChain->tableIndex;
                while (!equals(key, *currentNode->key) && currentChain)
                {
                    currentChain = currentChain->next;
                    if (currentChain)
                        currentNode = m_nodeTable + currentChain->tableIndex;
                }
                if (!currentBucket)
                    return;
                if (!equals(key, *currentNode->key))
                    return;
                if (currentChain->next)
                    currentChain->next->prev = currentChain->prev;
                if (currentChain->prev)
                    currentChain->prev->next = currentChain->next;
                if (currentBucket->firstChain == currentChain)
                    currentBucket->firstChain = currentChain->next;
                if (currentBucket->lastChain == currentChain)
                    currentBucket->lastChain = currentChain->prev;
                currentBucket->nChains--;

                if (currentNode->referencingNode->next)
                    currentNode->referencingNode->next->prev = currentNode->referencingNode->prev;
                if (currentNode->referencingNode->prev)
                    currentNode->referencingNode->prev->next = currentNode->referencingNode->next;
                if (m_head == currentNode->referencingNode)
                    m_head = currentNode->referencingNode->next;
                if (m_tail == currentNode->referencingNode)
                    m_tail = currentNode->referencingNode->prev;
                m_nNodes--;
                kfree(currentChain);
                currentNode->used = false;
                delete currentNode->key;
                delete currentNode->value;
                utils::memzero(currentNode, sizeof(*currentNode));
            }
            // This does nothing to the value stored in a node. Freeing any objects is the user's responsibility.
            void erase()
            {
                if (!m_nodeTable || !m_buckets)
                    return;
                for (auto _node = m_head; _node;)
                {
                    ((node*)_node->data)->used = false;
                    delete ((node*)_node->data)->key;
                    delete ((node*)_node->data)->value;
                    auto temp = _node->next;
                    delete _node;

                    _node = temp;
                }
                kfree(m_buckets); m_buckets = nullptr;
                kfree(m_nodeTable); m_nodeTable = nullptr;
                m_tableCapacity = 0;
            }

            size_t capacity() const
            { return m_tableCapacity; }
            void expand(size_t newCapacity)
            {
                if (newCapacity < m_tableCapacity)
                    return;
                m_tableCapacity = newCapacity;
                m_buckets   = (bucket*)krealloc(m_buckets  , m_tableCapacity * sizeof(bucket));
                m_nodeTable =   (node*)krealloc(m_nodeTable, m_tableCapacity * sizeof(node));
            }
            
            bool contains(const Key& key) const
            { return &at(key) != nullptr; }

            HashmapIter<Key, ValT, equals, hash> begin()
            {
                HashmapIter<Key, ValT, equals, hash> iter{ this, m_head };
                return iter;
            }
            HashmapIter<Key, ValT, equals, hash> end()
            {
                HashmapIter<Key, ValT, equals, hash> iter{ this, m_tail };
                return iter;
            }

            ~Hashmap()
            {
                erase();
            }

            friend HashmapIter<Key, ValT, equals, hash>;
        private:
            struct chain
            {
                size_t tableIndex;
                chain *next, *prev;
            };
            struct bucket
            {
                chain *firstChain, *lastChain;
                size_t nChains;
            };
            struct listNode
            {
                listNode *next, *prev;
                void* data;
            };
            struct node
            {
                const Key* key;
                ValT* value;
                struct listNode* referencingNode;
                bool used = false;
            };
            bucket* m_buckets = nullptr;
            node* m_nodeTable = nullptr;
            
            // These are used for iterators.

            listNode* m_head = nullptr;
            listNode* m_tail = nullptr;
            size_t m_nNodes;

            size_t m_tableCapacity = 64;
        };
    }
}