#pragma once
/*
static unsigned char header_data[] = {
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, 1, 1, 1, 1, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, B, 1, 2, 2, 1, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	B, 1, 1, 2, 2, 1, B, B, B, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B,
	1, 1, 2, 3, 2, 1, B, 1, 1, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, 1, 1, 1, B,
	1, 1, 2, 3, 2, 1, 1, 1, 1, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, B, 1, 2, 1, 1,
	1, 2, 3, 3, 2, 1, 1, 2, 1, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, 1, 1, 2, 1, 1,
	1, 2, 3, 3, 2, 1, 2, 2, 1, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, 1, 2, 2, 2, 1,
	2, 3, 3, 3, 2, 1, 2, 2, 1, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, 1, 3, 3, 2, 1,
	2, 3, 3, 3, 2, 2, 2, 2, 1, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, B, 1, 2, 3, 2, 2,
	2, 3, 3, 3, 2, 2, 3, 2, 1, B, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, 1, 1, 2, 3, 2, 2,
	3, 3, 4, 3, 2, 2, 3, 2, 1, 1, B, B, B, B, B, B,
	B, B, B, B, B, B, B, B, B, B, 1, 2, 2, 3, 2, 2,
	3, 4, 4, 3, 2, 2, 3, 2, 2, 1, 1, 1, 1, B, B, B,
	B, B, B, B, B, B, B, B, 1, 1, 1, 2, 3, 3, 3, 3,
	3, 4, 4, 3, 3, 2, 3, 3, 2, 1, 1, 1, 1, B, B, B,
	B, B, B, B, B, B, B, 1, 1, 2, 1, 2, 3, 4, 4, 4,
	4, 4, 4, 4, 3, 3, 3, 3, 2, 1, 1, 2, 1, 1, B, B,
	B, B, B, B, B, B, B, 1, 2, 2, 2, 2, 3, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 3, 2, 1, 1, 2, 2, 1, 1, B,
	B, B, B, B, B, B, B, 1, 2, 2, 2, 3, 3, 4, 4, 4,
	4, 4, 4, 4, 4, 4, 4, 3, 2, 1, 1, 2, 2, 1, 1, B,
	B, B, B, B, B, B, B, 1, 2, 2, 2, 3, 4, 4, 4, 5,
	5, 5, 5, 5, 5, 5, 4, 3, 2, 1, 1, 2, 2, 2, 1, B,
	B, B, B, B, B, B, 1, 1, 2, 3, 3, 4, 4, 4, 4, 5,
	5, 5, 5, 5, 5, 5, 4, 3, 2, 1, 2, 3, 3, 2, 1, B,
	B, B, B, B, B, B, 1, 2, 2, 3, 3, 4, 4, 4, 4, 5,
	5, 5, 5, 5, 5, 5, 4, 3, 2, 2, 2, 3, 3, 2, 1, B,
	B, B, B, B, B, B, 1, 2, 3, 3, 3, 4, 4, 4, 5, 5,
	5, 5, 5, 5, 5, 5, 4, 3, 3, 3, 3, 3, 3, 2, 1, B,
	B, B, B, B, B, B, 1, 2, 3, 3, 3, 4, 4, 4, 5, 5,
	5, 7, 7, 5, 5, 5, 4, 4, 4, 3, 3, 3, 3, 2, 1, B,
	B, B, B, B, B, 1, 1, 2, 3, 3, 3, 4, 4, 4, 5, 5,
	7, 7, 7, 5, 5, 5, 4, 4, 4, 3, 3, 3, 3, 2, 1, B,
	B, B, B, B, B, 1, 2, 3, 3, 3, 3, 4, 4, 4, 5, 7,
	7, 7, 7, 5, 5, 5, 4, 4, 4, 3, 3, 3, 3, 2, 1, B,
	B, B, B, B, B, 1, 2, 3, 3, 3, 3, 4, 4, 4, 5, 7,
	7, 7, 7, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 2, 1, B,
	B, B, B, B, B, 1, 2, 3, 3, 3, 3, 3, 4, 4, 5, 7,
	7, 7, 7, 7, 5, 4, 4, 4, 3, 3, 3, 3, 3, 2, 1, B,
	B, B, B, B, B, 1, 2, 3, 3, 3, 3, 3, 3, 4, 6, 7,
	7, 7, 7, 7, 4, 4, 4, 3, 3, 3, 3, 3, 2, 2, 1, B,
	B, B, B, B, B, 1, 1, 2, 2, 2, 3, 3, 3, 4, 6, 7,
	7, 7, 7, 7, 4, 4, 3, 3, 3, 3, 3, 2, 1, 1, 1, B,
	B, B, B, B, B, B, 1, 1, 1, 2, 2, 2, 3, 4, 7, 7,
	7, 7, 7, 7, 4, 4, 3, 3, 3, 2, 2, 1, 1, 1, B, B};
*/

static unsigned char flame_itex[] ALIGNED(64) = 
{
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0x11, 0x11, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0x21, 0x12, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0x1B, 0x21, 0x12, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 
0x11, 0x32, 0x12, 0x1B, 0xB1, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x11, 0xB1, 
0x11, 0x32, 0x12, 0x11, 0xB1, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x21, 0x11, 
0x21, 0x33, 0x12, 0x21, 0xB1, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x1B, 0x21, 0x11, 
0x21, 0x33, 0x12, 0x22, 0xB1, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x1B, 0x22, 0x12, 
0x32, 0x33, 0x12, 0x22, 0xB1, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x1B, 0x33, 0x12, 
0x32, 0x33, 0x22, 0x22, 0xB1, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x1B, 0x32, 0x22, 
0x32, 0x33, 0x22, 0x23, 0xB1, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x11, 0x32, 0x22, 
0x33, 0x34, 0x22, 0x23, 0x11, 0xBB, 0xBB, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0xBB, 0x21, 0x32, 0x22, 
0x43, 0x34, 0x22, 0x23, 0x12, 0x11, 0xB1, 0xBB, 
0xBB, 0xBB, 0xBB, 0xBB, 0x11, 0x21, 0x33, 0x33, 
0x43, 0x34, 0x23, 0x33, 0x12, 0x11, 0xB1, 0xBB, 
0xBB, 0xBB, 0xBB, 0x1B, 0x21, 0x21, 0x43, 0x44, 
0x44, 0x44, 0x33, 0x33, 0x12, 0x21, 0x11, 0xBB, 
0xBB, 0xBB, 0xBB, 0x1B, 0x22, 0x22, 0x43, 0x44, 
0x44, 0x44, 0x44, 0x34, 0x12, 0x21, 0x12, 0xB1, 
0xBB, 0xBB, 0xBB, 0x1B, 0x22, 0x32, 0x43, 0x44, 
0x44, 0x44, 0x44, 0x34, 0x12, 0x21, 0x12, 0xB1, 
0xBB, 0xBB, 0xBB, 0x1B, 0x22, 0x32, 0x44, 0x54, 
0x55, 0x55, 0x55, 0x34, 0x12, 0x21, 0x22, 0xB1, 
0xBB, 0xBB, 0xBB, 0x11, 0x32, 0x43, 0x44, 0x54, 
0x55, 0x55, 0x55, 0x34, 0x12, 0x32, 0x23, 0xB1, 
0xBB, 0xBB, 0xBB, 0x21, 0x32, 0x43, 0x44, 0x54, 
0x55, 0x55, 0x55, 0x34, 0x22, 0x32, 0x23, 0xB1, 
0xBB, 0xBB, 0xBB, 0x21, 0x33, 0x43, 0x44, 0x55, 
0x55, 0x55, 0x55, 0x34, 0x33, 0x33, 0x23, 0xB1, 
0xBB, 0xBB, 0xBB, 0x21, 0x33, 0x43, 0x44, 0x55, 
0x75, 0x57, 0x55, 0x44, 0x34, 0x33, 0x23, 0xB1, 
0xBB, 0xBB, 0x1B, 0x21, 0x33, 0x43, 0x44, 0x55, 
0x77, 0x57, 0x55, 0x44, 0x34, 0x33, 0x23, 0xB1, 
0xBB, 0xBB, 0x1B, 0x32, 0x33, 0x43, 0x44, 0x75, 
0x77, 0x57, 0x55, 0x44, 0x34, 0x33, 0x23, 0xB1, 
0xBB, 0xBB, 0x1B, 0x32, 0x33, 0x43, 0x44, 0x75, 
0x77, 0x57, 0x45, 0x44, 0x34, 0x33, 0x23, 0xB1, 
0xBB, 0xBB, 0x1B, 0x32, 0x33, 0x33, 0x44, 0x75, 
0x77, 0x77, 0x45, 0x44, 0x33, 0x33, 0x23, 0xB1, 
0xBB, 0xBB, 0x1B, 0x32, 0x33, 0x33, 0x43, 0x76, 
0x77, 0x77, 0x44, 0x34, 0x33, 0x33, 0x22, 0xB1, 
0xBB, 0xBB, 0x1B, 0x21, 0x22, 0x33, 0x43, 0x76, 
0x77, 0x77, 0x44, 0x33, 0x33, 0x23, 0x11, 0xB1, 
0xBB, 0xBB, 0xBB, 0x11, 0x21, 0x22, 0x43, 0x77, 
0x77, 0x77, 0x44, 0x33, 0x23, 0x12, 0x11, 0xBB
};
