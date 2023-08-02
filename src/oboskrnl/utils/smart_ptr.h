/*
	smart_ptr.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

namespace obos
{
	namespace utils
	{
		template<typename T>
		struct DefaultAllocator
		{
			T* alloc(const T& value)
			{
				return new T{ value };
			}
			T* alloc()
			{
				return new T{};
			}
			void free(T* block)
			{
				delete block;
			}
			DefaultAllocator() = default;
			~DefaultAllocator() = default;
		};
		// "typename allocator" must implement three functions:
		// T*   alloc(const T& value);
		// T*   alloc();
		// void free(T* block);

		template<typename T, typename allocator = DefaultAllocator<T>>
		class SmartPtr
		{
		public:
			SmartPtr();
			SmartPtr(const T& defaultValue);
			
			SmartPtr(SmartPtr&&) = delete;
			SmartPtr(const SmartPtr&) = delete;

			T* get();

			T& operator*();
			T* operator->();

			void release();
			virtual ~SmartPtr();
		private:
			T* m_object = nullptr;
			allocator m_allocator;
		};

		template<typename T, typename allocator>
		inline SmartPtr<T, allocator>::SmartPtr()
		{
			m_allocator.alloc(T());
		}
		template<typename T, typename deleter>
		inline SmartPtr<T, deleter>::SmartPtr(const T& defaultValue)
		{
			m_allocator.alloc(defaultValue);
		}

		template<typename T, typename allocator>
		inline T* SmartPtr<T, allocator>::get()
		{
			return m_object;
		}

		template<typename T, typename allocator>
		inline T& SmartPtr<T, allocator>::operator*()
		{
			return *m_object;
		}
		template<typename T, typename allocator>
		inline T* SmartPtr<T, allocator>::operator->()
		{
			return m_object;
		}

		template<typename T, typename allocator>
		inline void SmartPtr<T, allocator>::release()
		{
			m_allocator.free(m_object);
		}
		template<typename T, typename allocator>
		inline SmartPtr<T, allocator>::~SmartPtr()
		{
			release();
		}
	}
}