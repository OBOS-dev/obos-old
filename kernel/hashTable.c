#include "hashTable.h"

#include "liballoc/liballoc_1_1.h"

struct hash_table_node
{
	BOOL inUse;
	SIZE_T index;
	struct key key;
	PVOID data;
	SIZE_T data_size;
};
typedef struct __hash_table
{
	struct hash_table_node* nodes;
	SIZE_T size;
	SIZE_T capacity;
	int(*hash)(struct key* this);
	int(*compare)(struct key* right, struct key* left);
} hash_table;

hash_table* hash_table_create(SIZE_T initialCapacity, int(*hash)(struct key* _this), int(*compare)(struct key* right, struct key* left))
{
	hash_table* ret = kmalloc(sizeof(hash_table));
	if (!ret)
		return NULLPTR;
	ret->nodes = kmalloc(initialCapacity * sizeof(struct hash_table_node));
	if (!ret->nodes)
		return NULLPTR;
	ret->capacity = initialCapacity;
	ret->size = 0;
	ret->hash = hash;
	ret->compare = compare;
	return ret;
}
BOOL hash_table_emplace(hash_table* table, struct key key, PVOID data, SIZE_T size)
{
	struct hash_table_node node;
	node.data = kmalloc(size);
	if (!node.data)
		return FALSE;
	{
		char* temp1 = data;
		char* temp2 = node.data;
		for (int i = 0; i < size; i++) temp2[i] = temp1[i];
	}
	node.key = key;
	node.index = table->hash(&key) % table->capacity;
	node.inUse = TRUE;
	node.data_size = size;
	for (; table->nodes[node.index].inUse && table->compare(&table->nodes[node.index].key, &key) != 0 && node.index < table->capacity; node.index++);
	if (table->nodes[node.index].inUse && table->compare(&table->nodes[node.index].key, &key) != 0)
	{
		node.index++;
		PVOID newNodes = krealloc(table->nodes, table->capacity * 1.25f);
		if (!newNodes)
			return FALSE;
		table->nodes = newNodes;
		table->capacity *= 1.25f;
	}
	BOOL amountToAdd = TRUE;
	if (table->nodes[node.index].inUse && table->compare(&table->nodes[node.index].key, &key) == 0)
	{
		kfree(table->nodes[node.index].data);
		if (table->nodes[node.index].key.destroy_key)
			table->nodes[node.index].key.destroy_key(table->nodes[node.index].key.key);
		amountToAdd = FALSE;
	}
	table->nodes[node.index] = node;
	table->size += amountToAdd;
	return TRUE;
}
BOOL hash_table_lookup(hash_table* table, struct key key, PVOID* data, SIZE_T* size)
{
	int index = table->hash(&key) % table->capacity;
	struct hash_table_node* node = &table->nodes[index];
	if (!node->inUse)
		return FALSE;
	if (table->compare(&node->key, &key) != 0)
		for (; table->compare(&node->key, &key) != 0 && index < table->capacity; node = table->nodes + index++);
	if (index == table->capacity && table->compare(&node->key, &key) != 0)
		return FALSE;
	*size = node->data_size;
	if (size != NULLPTR && *size != 0)
		if (*size >= node->data_size)
			*data = node->data;
		else
			return FALSE;
	else
			*data = node->data;
	return TRUE;
}
BOOL hash_table_contains(hash_table* table, struct key key, BOOL* out)
{
	*out = FALSE;
	int index = table->hash(&key) % table->capacity;
	struct hash_table_node* node = &table->nodes[index];
	if (!node->inUse)
		return TRUE;
	if (table->compare(&node->key, &key) != 0)
		for (; table->compare(&node->key, &key) != 0 && index < table->capacity; node = table->nodes + index++);
	if (index == table->capacity && table->compare(&node->key, &key) != 0)
		return TRUE;
	*out = TRUE;
	return TRUE;
}
void hash_table_destroy(hash_table** table)
{
	for (int i = 0; i < (*table)->capacity; i++)
	{
		struct hash_table_node* node = &((*table)->nodes[i]);
		if (!node->inUse)
			continue;
		if (node->key.destroy_key)
			node->key.destroy_key(node->key.key);
		kfree(node->data);
	}
	kfree((*table)->nodes);
	kfree(*table);
	*table = NULLPTR;
}
