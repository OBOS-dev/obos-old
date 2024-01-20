/*
    drivers/x86_64/common/new.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <allocators/liballoc.h>

[[nodiscard]] void* operator new(size_t count) noexcept
{
	return kmalloc(count);
}
[[nodiscard]] void* operator new[](size_t count) noexcept
{
	return operator new(count);
}
void operator delete(void* block) noexcept
{
	kfree(block);
}
void operator delete[](void* block) noexcept
{
	kfree(block);
}
void operator delete(void* block, size_t)
{
	kfree(block);
}
void operator delete[](void* block, size_t)
{
	kfree(block);
}

[[nodiscard]] void* operator new(size_t, void* ptr) noexcept
{
	return ptr;
}
[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept
{
	return ptr;
}
void operator delete(void*, void*) noexcept
{}
void operator delete[](void*, void*) noexcept
{}