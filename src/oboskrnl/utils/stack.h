/*
	oboskrnl/utils/stack.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <new>

#include <allocators/liballoc.h>

namespace obos
{
	namespace utils
	{
		// These stack classes are FIFO (first in, first out)

		// Recommended for bigger objects.
		template<typename T>
		class ListStack
		{
		public:
			ListStack() = default;
			
			void push(const T& obj)
			{
				StackNode* newNode = new StackNode{ obj };
				newNode->next = m_head;
				m_head = newNode;
				m_nNodes++;
			}
			void pop()
			{
				if (!m_head)
					return;
				m_nNodes--;
				auto next = m_head->next;
				delete m_head;
				m_head = next;
			}
			T& get()
			{
				if (!m_head)
					return *(T*)nullptr;
				return m_head->data;
			}

			void erase()
			{
				while (m_head)
				{
					m_nNodes--;
					auto next = m_head->next;
					delete m_head;
					m_head = next;
				}
			}

			~ListStack()
			{
				erase();
			}
		private:
			struct StackNode
			{
				StackNode() = delete;
				StackNode(const T& obj)
					: data{obj} {}
				T data;
				StackNode *next;
			};
			StackNode *m_head = nullptr;
			size_t m_nNodes = 0;
		};
		// Recommended for smaller objects.
		template<typename T>
		class ArrayStack
		{
		public:
			ArrayStack() = default;

			void push(const T& obj)
			{
				m_data = (T*)krealloc(m_data, m_szData++);
				m_currentOffset = m_szData;
				new (&m_data[m_currentOffset]) T{ obj };
			}
			void pop()
			{
				if (!m_szData)
					return;
				::operator delete(m_data, m_data + m_currentOffset);
				m_data = (T*)krealloc(m_data, m_szData--);
				m_currentOffset = m_szData;
			}
			T& get()
			{
				return m_data[m_currentOffset];
			}

			void erase()
			{
				if (!m_szData)
					return;
				while (m_currentOffset--)
					::operator delete(m_data, m_data + m_currentOffset);
				kfree(m_data);
				m_szData = 0;
			}

			~ArrayStack()
			{
				erase();
			}
		private:
			T* m_data;
			size_t m_currentOffset = 0;
			size_t m_szData = 0;
		};
	}
}