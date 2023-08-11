/*
	utils/hashTable.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#include <utils/memory.h>

namespace obos
{
	namespace utils
	{
		template <typename keyType, typename dataType>
		struct hashTableNode
		{
			keyType key;
			dataType data;
			bool used = false;
		};
		template <typename keyType>
		class default_hasher
		{
		public:
			static UINT32_T hash(const keyType& key)
			{
				PBYTE data = (PBYTE)&key;

				UINT32_T hash = 0;

				for (int i = 0, c = data[0]; i < sizeof(keyType); c = data[i++])
					hash = c + (hash << 6) + (hash << 16) - hash;

				return hash;
			}
		};
		template <typename keyType, typename dataType, class hasher = default_hasher<keyType>>
		class HashTable
		{
		public:
			HashTable()
			{
				m_capacity = 16;
				m_size = 0;
				m_nodes = new hashTableNode<keyType, dataType>[m_capacity];
			}
			HashTable(SIZE_T capacity)
			{
				m_capacity = capacity;
				m_size = 0;
				m_nodes = new hashTableNode<keyType, dataType>[m_capacity];
				utils::memzero(m_nodes, sizeof(hashTableNode<keyType, dataType>) * m_capacity);
			}

			HashTable(const HashTable& _other)
			{
				this->m_capacity = _other.m_capacity;
				this->m_size = _other.m_size;
				m_nodes = (hashTableNode<keyType, dataType>*)utils::memcpy(new hashTableNode<keyType, dataType>[m_capacity], this->m_nodes, m_capacity);
			}

			const dataType& at(const keyType& key) const
			{
				auto hashVal = hasher::hash(key) % m_capacity;
				
				if (hashVal > m_capacity)
					return dataType{};

				auto dataVal = m_nodes[hashVal];

				if (!dataVal.used)
					return dataType{};

				if (memcmp(&dataVal.key, &key, sizeof(keyType)) != 0)
					for (int i = hashVal; i < m_capacity && memcmp(&dataVal.key, &key, sizeof(keyType)); dataVal = m_nodes[i++]);

				return dataVal.data;
			}

			bool contains(const keyType& key) const
			{
				auto hashVal = hasher::hash(key) % m_capacity;
				return m_nodes[hashVal].used;
			}

			// Returns the this->at(key) before it gets emplaced.
			dataType& emplace(const keyType& key, const dataType& data)
			{
				auto hashVal = hasher::hash(key) % m_capacity;

				if (hashVal > m_capacity)
				{
					auto oldCapacity = m_capacity;
					m_capacity + m_capacity / 4;
					hashTableNode<keyType, dataType>* newNodes = new hashTableNode<keyType, dataType>[m_capacity];
					utils::memcpy(newNodes, m_nodes, oldCapacity);
					delete[] m_nodes;
					m_nodes = newNodes;
				}

				auto& dataVal = m_nodes[hashVal];
			
				if (memcmp(&dataVal.key, &key, sizeof(keyType)) != 0)
					for (int i = hashVal; i < m_capacity && memcmp(&dataVal.key, &key, sizeof(keyType)); dataVal = m_nodes[i++]);

				dataType oldVal = dataVal.data;

				dataVal.used = true;
				dataVal.key = key;
				dataVal.data = data;

				return oldVal;
			}

			// Returns data if it gets emplaced, otherwise the current value at key.
			dataType& try_emplace(const keyType& key, dataType& data)
			{
				if (!contains(key))
				{
					emplace(key, data);
					return data;
				}
				return (dataType&)at(key);
			}

			dataType& operator[](const keyType& key)
			{
				auto hashVal = hasher::hash(key) % m_capacity;

				auto& dataVal = m_nodes[hashVal];

				if (memcmp(&dataVal.key, &key, sizeof(keyType)) != 0)
					for (int i = hashVal; i < m_capacity && memcmp(&dataVal.key, &key, sizeof(keyType)); dataVal = m_nodes[i++]);

				if (!dataVal.used)
				{
					dataVal.key = key;
					dataVal.used = true;
				}

				return dataVal.data;
			}

			virtual ~HashTable()
			{
				delete[] m_nodes;
			}
		private:
			hashTableNode<keyType, dataType>* m_nodes = nullptr;
			SIZE_T m_size = 0;
			SIZE_T m_capacity = 0;
		};
	}
}