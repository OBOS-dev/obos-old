/*
	oboskrnl/vector.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

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

			while (*_key)
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
            Hashmap()
            {
                m_buckets = (bucket*)kcalloc(m_tableCapacity, sizeof(bucket));
                m_nodeTable = (node*)kcalloc(m_tableCapacity, sizeof(node));
            }

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
                while (!equals(key, currentNode->key) && currentChain)
                {
                    currentChain = currentChain->next;
                    if (currentChain)
                        currentNode = m_nodeTable + currentChain->tableIndex;
                }
                if (!currentBucket)
                    return *(ValT*)nullptr;
                if (!equals(key, currentNode->key))
                    return *(ValT*)nullptr;
                return currentNode->value;
            }

            void emplace_at(const Key& key, ValT& value)
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
                    capacity(m_tableCapacity = 64);
                if (m_nodeTable[ourIndex].used)
                {
                    capacity(m_tableCapacity *= 2);
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
                ourBucket->nChains++;
                m_nodeTable[ourIndex].key   = key;
                m_nodeTable[ourIndex].value = value;
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
                node* currentNode = m_nodeTable + currentChain->index;
                while (!equals(key, currentNode->key) && currentChain)
                {
                    currentChain = currentBucket->next;
                    if (currentChain)
                        currentNode = m_nodeTable + currentChain->index;
                }
                if (!currentBucket)
                    return;
                if (!equals(key, currentNode->key))
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
                kfree(currentChain);
                currentNode->used = false;
                utils::memzero(currentNode, sizeof(*currentNode));
            }
            // This does nothing to the value stored in a node. Freeing any objects is the user's responsibility.
            void erase()
            {
                if (!m_nodeTable || !m_buckets)
                    return;
                for (size_t i = 0; i < m_tableCapacity; i++)
                {
                    if (m_buckets[i].nChains)
                    {
                        for (chain* node = m_buckets[i].firstChain; node; )
                        {
                            chain* temp = node->next;
                            kfree(node);
                            node = temp;
                            m_buckets[i].nChains--;
                        }
                    }
                }
                kfree(m_buckets); m_buckets = nullptr;
                kfree(m_nodeTable); m_nodeTable = nullptr;
                m_tableCapacity = 0;
            }

            size_t capacity() const
            { return m_tableCapacity; }
            void capacity(size_t newCapacity)
            {
                if (newCapacity < m_tableCapacity)
                    return;
                m_tableCapacity = newCapacity;
                m_buckets   = (bucket*)krealloc(m_nodeTable, m_tableCapacity * sizeof(bucket));
                m_nodeTable =   (node*)krealloc(m_nodeTable, m_tableCapacity * sizeof(node));
            }
            
            bool contains(const Key& key) const
            { return &at(key) != nullptr; }

            ~Hashmap()
            {
                erase();
            }
        private:
            struct node
            {
                const Key& key;
                ValT& value;
                bool used = false;
            };
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
            bucket* m_buckets = nullptr;
            node* m_nodeTable = nullptr;
            size_t m_tableCapacity = 64;
        };
    }
}