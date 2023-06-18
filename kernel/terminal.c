#include "terminal.h"
#include "types.h"
#include "inline-asm.h"

#include "multiboot/mutliboot.h"

#define vga_entry(uc, color) ((UINT16_T) uc | (UINT16_T) color << 8)
#define terminal_putentryat(c, color, x,y) terminal_buffer[(y) * VGA_WIDTH + (x)] = vga_entry((c), (color))

static const SIZE_T VGA_WIDTH = 80;
static const SIZE_T VGA_HEIGHT = 25;
 
static SIZE_T terminal_row;
static SIZE_T terminal_column;
static UINT8_T terminal_color;
static UINT16_T* terminal_buffer;

extern multiboot_info_t* g_multibootInfo;

static inline void* memsetb(void* destination, BYTE value, SIZE_T countBytes)
{
	CHAR* _dest = destination;
	for(int i = 0; i < countBytes; i++, _dest[i] = value);
	return destination;
}
static inline void* memsetw(void* destination, UINT16_T value, SIZE_T count)
{
	UINT16_T* _dest = destination;
	for(int i = 0; i < count; i++, _dest[i] = value);
	return destination;
}

void SetTerminalCursorPosition(CONSOLEPOINT point)
{
	UINT16_T pos = point.y * VGA_WIDTH + point.x;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (UINT8_T) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (UINT8_T) ((pos >> 8) & 0xFF));

    terminal_column = point.x;
    terminal_row = point.y;
}
CONSOLEPOINT GetTerminalCursorPosition()
{
    CONSOLEPOINT ret;
    ret.x = terminal_column;
    ret.y = terminal_row;
    return ret;
}

void InitializeTeriminal(UINT8_T color) 
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | 7);
	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | 9);
	
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = color;
	terminal_buffer = (UINT16_T*)(UINTPTR_T)g_multibootInfo->framebuffer_addr;
	for (SIZE_T y = 0; y < VGA_HEIGHT; y++) {
		for (SIZE_T x = 0; x < VGA_WIDTH; x++)
			terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
	}
}
 
void TerminalSetColor(UINT8_T color) 
{
	terminal_color = color;
}
UINT8_T TerminalGetColor() 
{
	return terminal_color;
}

static BOOL g_reachedEndTerminal = 0;

static void newline_handler()
{
	if(++terminal_row >= VGA_HEIGHT)
	{
		g_reachedEndTerminal = TRUE;
		memsetb(terminal_buffer, 0, (VGA_WIDTH - 1) * sizeof(UINT16_T));
		for(int y = 0, y2 = 1; y < VGA_HEIGHT; y++, y2++)
			for(int x = 0; x < VGA_WIDTH; x++)
				terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[y2 * VGA_WIDTH + x];
		terminal_row = VGA_HEIGHT - 1;
	}
	CONSOLEPOINT point;
	point.x = terminal_column;
	if(!g_reachedEndTerminal)
		point.y = terminal_row;
	else
	{ 
		point.y = VGA_HEIGHT - 1; 
		terminal_row = VGA_HEIGHT - 1; 
	}
	SetTerminalCursorPosition(point);
}

void TerminalOutputCharacter(CHAR c)
{
	if(c == '\n')
	{
		newline_handler();
		return;
	}
	else if (c == '\r')
	{
		terminal_column = 0;
		return;
	}
	else if (c == '\t')
	{
		for(int i = 0; i < 5 && terminal_column < VGA_WIDTH; i++)
			terminal_putentryat(' ', terminal_color, ++terminal_column, terminal_row);
		return;
	}
	else if (c == '\0')
		return;
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH)
	{
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			newline_handler();
	}
	CONSOLEPOINT point;
	point.x = terminal_column;
	if(!g_reachedEndTerminal)
    	point.y = terminal_row;
    SetTerminalCursorPosition(point);
}
void TerminalOutputCharacterAt(CONSOLEPOINT point, CHAR c)
{
	terminal_putentryat(c, terminal_color, point.x, point.y);
}
void TerminalOutput(CSTRING data, SIZE_T size)
{
	for (SIZE_T i = 0; i < size; i++)
		TerminalOutputCharacter(data[i]);
}
void TerminalOutputString(CSTRING data)
{
	SIZE_T i = 0;
	for(; data[i]; i++);
	TerminalOutput(data, i);
}