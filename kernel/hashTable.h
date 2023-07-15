/*
	hashTable.h

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_HASHTABLE_H
#define __OBOS_HASHTABLE_H

#include "types.h"

#define hash_table_emplace_type(_table, _key, key_type, key_size, value, value_size) \
{\
	struct key ___temp_key;\
	___temp_key.key = kcalloc(1, key_size);\
	*(key_type*)___temp_key.key = _key;\
	___temp_key.destroy_key = kfree;\
	hash_table_emplace(_table, ___temp_key, (PVOID)&value, value_size);\
}
#define hash_table_lookup_type(_table, _key, key_type, key_size, data, data_size) \
{\
	struct key ___temp_key;\
	___temp_key.key = kcalloc(1, key_size);\
	*(key_type*)___temp_key.key = _key;\
	hash_table_lookup(_table, ___temp_key, (PVOID*)&data, (SIZE_T*)&data_size);\
	kfree(___temp_key.key);\
}
#define hash_table_contains_type(_table, _key, key_type, key_size, out) \
{\
	struct key ___temp_key;\
	___temp_key.key = kcalloc(1, key_size);\
	*(key_type*)___temp_key.key = _key;\
	hash_table_contains(_table, ___temp_key, out);\
	free_function(___temp_key.key);\
}

typedef struct __hash_table hash_table;
struct key
{
	void* key;
	void(*destroy_key)(void* block);
};

hash_table* hash_table_create(SIZE_T initialCapacity, int(*hash)(struct key* _this), int(*compare)(struct key* right, struct key* left));
BOOL hash_table_emplace(hash_table* table, struct key key, PVOID data, SIZE_T size);
BOOL hash_table_lookup(hash_table* table, struct key key, PVOID* data, SIZE_T* size);
BOOL hash_table_contains(hash_table* table, struct key key, BOOL* out);
void hash_table_destroy(hash_table** table);

#endif