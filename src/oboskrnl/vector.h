/*
	oboskrnl/vector.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#ifdef OBOS_DEBUG
#include <klog.h>
#endif

#include <memory_manipulation.h>

#include <allocators/liballoc.h>

namespace obos
{
	namespace utils
	{
		template<typename T>
		class Vector
		{
		public:
			template<typename _IT>
			class VectorIter
			{
			public:
				VectorIter() = delete;

				_IT& operator*()
				{
					return m_vec->operator[](m_index);
				}
				size_t operator++()
				{
					return ++m_index;
				}
				size_t operator++(int)
				{
					return m_index++;
				}
				size_t operator--()
				{
					return --m_index;
				}
				size_t operator--(int)
				{
					return m_index--;
				}

				operator bool() { return m_index < m_vec->m_sz; }

				friend Vector<T>;
			private:
				VectorIter(const Vector<_IT>* vec, size_t startingIndex)
				{
					m_vec = vec;
					m_index = startingIndex;
				}
				size_t m_index = 0;
				const Vector<_IT>* m_vec;
			};

			Vector() 
			{ 
				m_capacity = 0;
			}

			T& push_back()
			{
				if ((m_sz + 1) >= m_capacity)
				{
					m_capacity += m_capacityIncrement;
					if (m_ptr)
						m_ptr = (T*)krealloc(m_ptr, m_capacity * sizeof(T));
					else
						m_ptr = new T[m_capacity];
				}
				return m_ptr[m_sz++];
			}
			T& push_back(const T& val)
			{
				if ((m_sz + 1) > m_capacity)
				{
					m_capacity += m_capacityIncrement;
					m_ptr = (T*)krealloc(m_ptr, m_capacity * sizeof(T));
				}
#ifdef OBOS_DEBUG
				OBOS_ASSERTP(m_ptr > (void*)0xfffffffff0000000, "");
#endif
				return m_ptr[m_sz++] = val;
			}
			void pop_back()
			{
				if (!m_sz)
					return;
				if (--m_sz < m_capacity)
				{
					m_capacity -= m_capacityIncrement;
					if (m_capacity)
						m_ptr = (T*)krealloc(m_ptr, m_capacity * sizeof(T));
				}
				else
					m_ptr[m_sz] = T{};
			}

			size_t length() { return m_sz; }
			size_t capacity() { return m_capacity; }
			// Checks if val is the invalid value returned by at and operator[]
			static bool isInvalidElement(T& val)
			{
				return &val == &m_invalidValue;
			}

			T& at(size_t i) const
			{
				if (i >= m_sz)
					return m_invalidValue;
				return m_ptr[i];
			}

			T& operator[](size_t i) const
			{
				return at(i);
			}
			T* operator+(size_t offset) const
			{
				return m_ptr + offset;
			}

			void clear()
			{
				if (m_ptr)
					delete[] m_ptr;
				m_ptr = nullptr;
				m_sz = 0;
				m_capacity = 0;
			}

			const T* data() const
			{ return m_ptr; }

			T& front() const
			{ return at(0); }
			T& back() const
			{ return at(m_sz - 1); }

			VectorIter<T> begin() const
			{
				VectorIter<T> iter{ this, 0 };
				return iter;
			}
			VectorIter<T> end() const
			{
				VectorIter<T> iter{ this, m_sz };
				return iter;
			}

			~Vector()
			{
				if (m_ptr)
					delete[] m_ptr;
			}
			friend VectorIter<T>;
		private:
			T* m_ptr = 0;
			size_t m_sz = 0;
			size_t m_capacity = 0;
			static inline T m_invalidValue;
			static inline constexpr size_t m_capacityIncrement = 4;
		};
	}
}