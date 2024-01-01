/*
    oboskrnl/driverInterface/register_drive.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <driverInterface/register_drive.h>

#include <vector.h>

#include <allocators/liballoc.h>

namespace obos
{
	namespace driverInterface
	{
        struct drive
        {
            uint32_t id = 0;
            drive *next, *prev;
        };
        struct 
        {
            drive *head = nullptr, *tail = nullptr;
            size_t len = 0;
            void push(drive& node)
            {
                if (tail)
                    tail->next = &node;
                if(!head)
                    head = &node;
                node.prev = tail;
                len++;
            }
            void remove(drive& node, void(*freer)(void*))
            {
                if(node.next)
                    node.next->prev = node.prev;
                if(node.next)
                    node.prev->next = node.next;
                if (head == &node)
                    head = node.next;
                if (tail == &node)
                    tail = node.prev;
                len--;
                if (freer)
                    freer(&node);
            }
        } g_drives;
        
		uint32_t RegisterDrive()
        {
            drive* node = new drive{};
            node->id = g_drives.len;
            g_drives.push(*node);
        }
		void UnregisterDrive(uint32_t id)
        {
            drive* node = nullptr;
            for (drive* cnode = g_drives.head; cnode;)
            {
                if (cnode->id == id)
                {
                    node = cnode;
                    break;
                }

                cnode = cnode->next;
            }
            if (!node)
                return;
            g_drives.remove(*node, kfree);
        }
	}
}