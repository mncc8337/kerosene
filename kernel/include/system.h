#pragma once

#define VIDEO_ADDRESS 0xb8000
// device I/O ports
#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

unsigned char* memcpy(unsigned char* dest, unsigned char* src, int cnt);
unsigned char* memset(unsigned char* dest, unsigned char val, int cnt);
unsigned short* memsetw(unsigned short* dest, unsigned short cal, int cnt);
int strlen(const char* str);

unsigned char port_inb(unsigned short port);
void port_outb(unsigned short port, unsigned char data);
unsigned short port_inw(unsigned short port);
void port_outw(unsigned short port, unsigned short data);
