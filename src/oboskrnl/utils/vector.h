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
				m_sz = 0;
				m_ptr = nullptr;
			}

			Vector(const Vector& other)
			{
				if (!other.m_capacity)
					return;
				m_capacity = other.m_capacity;
				m_sz       = other.m_sz;
				m_ptr      = new T[m_capacity];
				for (size_t i = 0; i < m_sz; i++)
					m_ptr[i] = other.m_ptr[i];
			}
			Vector& operator=(const Vector& other)
			{
				if (!other.m_capacity)
					return *this;
				m_capacity = other.m_capacity;
				m_sz       = other.m_sz;
				m_ptr      = new T[m_capacity];
				for (size_t i = 0; i < m_sz; i++)
					m_ptr[i] = other.m_ptr[i];
				return *this;
			}
			Vector(Vector&& other)
			{
				if (!other.m_capacity)
					return;
				m_capacity = other.m_capacity;
				m_sz       = other.m_sz;
				m_ptr      = new T[m_capacity];
				for (size_t i = 0; i < m_sz; i++)
					m_ptr[i] = other.m_ptr[i];
				if (other.m_ptr)
					delete m_ptr;
				other.m_ptr = nullptr;
				other.m_sz = 0;
				other.m_capacity = 0;
			}
			Vector& operator=(Vector&& other)
			{
				if (!other.m_capacity)
					return *this;
				m_capacity = other.m_capacity;
				m_sz       = other.m_sz;
				m_ptr      = new T[m_capacity];
				for (size_t i = 0; i < m_sz; i++)
					m_ptr[i] = other.m_ptr[i];
				if (other.m_ptr)
					delete m_ptr;
				other.m_ptr = nullptr;
				other.m_sz = 0;
				other.m_capacity = 0;
				return *this;
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
				if ((m_sz + 1) > m_capacity || !m_ptr)
				{
					m_capacity += m_capacityIncrement;
					m_ptr = (T*)krealloc(m_ptr, m_capacity * sizeof(T));
				}
#ifdef OBOS_DEBUG
				OBOS_ASSERTP(m_ptr != nullptr, "");
#endif
				m_ptr[m_sz++] = val;
				return m_ptr[m_sz - 1];
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
			}

			size_t length() const { return m_sz; }
			size_t capacity() const { return m_capacity; }
			// Checks if val is the invalid value returned by at and operator[]
			static bool isInvalidElement(T& val)
			{
				return &val == nullptr;
			}

			T& at(size_t i) const
			{
				if (i >= m_sz)
					return *((T*)nullptr);
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
					delete m_ptr;
				m_ptr = nullptr;
				m_sz = 0;
				m_capacity = 0;
			}

			// The parameter returned by this function should not be stored anywhere, as it is subject
			// to reallocations that move the vector's data.
			T* data() const
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

			void resize(size_t newSize)
			{
				m_sz = newSize;
				if (m_sz < m_capacity)
					return;
				m_capacity = m_sz + (m_sz - (m_sz % m_capacityIncrement));
				m_ptr = (T*)krealloc(m_ptr, m_capacity * sizeof(T));
			}

			~Vector()
			{
				if (m_ptr)
					delete m_ptr;
			}
			friend VectorIter<T>;
		private:
			T* m_ptr = 0;
			size_t m_sz = 0;
			size_t m_capacity = 0;
			static inline constexpr size_t m_capacityIncrement = 4;
		};
	}
}