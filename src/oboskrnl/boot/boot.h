/*
	boot/boot.h

	Copyright (c) 2023 Omar Berrow
*/

#include <boot/multiboot.h>

namespace obos
{
#ifdef __x86_64__
    struct multiboot_mod_list64
    {
        /* the memory used goes from bytes ’mod_start’ to ’mod_end-1’ inclusive */
        multiboot_uint64_t mod_start;
        multiboot_uint64_t mod_end;

        /* Module command line */
        multiboot_uint64_t cmdline;

        /* padding to take it to 16 bytes (must be zero) */
        multiboot_uint32_t pad;
    };

    struct multiboot_info
    {
        /* Multiboot info version number */
        multiboot_uint32_t flags;

        /* Available memory from BIOS */
        multiboot_uint64_t mem_lower;
        multiboot_uint64_t mem_upper;

        /* "root" partition */
        multiboot_uint64_t boot_device;

        /* Kernel command line */
        multiboot_uint64_t cmdline;

        /* Boot-Module list */
        multiboot_uint64_t mods_count;
        multiboot_uint64_t mods_addr;

        union
        {
            multiboot_aout_symbol_table_t aout_sym;
            multiboot_elf_section_header_table_t elf_sec;
        } u;

        /* Memory Mapping buffer */
        multiboot_uint64_t mmap_length;
        multiboot_uint64_t mmap_addr;

        /* Drive Info buffer */
        multiboot_uint64_t drives_length;
        multiboot_uint64_t drives_addr;

        /* ROM configuration table */
        multiboot_uint64_t config_table;

        /* Boot Loader Name */
        multiboot_uint64_t boot_loader_name;

        /* APM table */
        multiboot_uint64_t apm_table;

        /* Video */
        multiboot_uint64_t vbe_control_info;
        multiboot_uint64_t vbe_mode_info;
        multiboot_uint16_t vbe_mode;
        multiboot_uint16_t vbe_interface_seg;
        multiboot_uint16_t vbe_interface_off;
        multiboot_uint16_t vbe_interface_len;

        multiboot_uint64_t framebuffer_addr;
        multiboot_uint32_t framebuffer_pitch;
        multiboot_uint32_t framebuffer_width;
        multiboot_uint32_t framebuffer_height;
        multiboot_uint8_t framebuffer_bpp;
        multiboot_uint8_t framebuffer_type;
        union
        {
            struct
            {
                multiboot_uint32_t framebuffer_palette_addr;
                multiboot_uint16_t framebuffer_palette_num_colors;
            };
            struct
            {
                multiboot_uint8_t framebuffer_red_field_position;
                multiboot_uint8_t framebuffer_red_mask_size;
                multiboot_uint8_t framebuffer_green_field_position;
                multiboot_uint8_t framebuffer_green_mask_size;
                multiboot_uint8_t framebuffer_blue_field_position;
                multiboot_uint8_t framebuffer_blue_mask_size;
            };
        };
    };
    using multiboot_info_t = obos::multiboot_info;
    using multiboot_module_t = obos::multiboot_mod_list64;
    extern obos::multiboot_info_t* g_multibootInfo;
#elif defined(__i686__)
    using multiboot_info_t = ::multiboot_info_t;
    extern obos::multiboot_info_t* g_multibootInfo;
#endif
}