#include "git.h"

#include <stdio.h>
#include <fcntl.h>
#include <kernel.h>
#include <graph.h>
#include <gs_psm.h>
#include <libpad.h>

#include <ee_regs.h>
#include <vif_codes.h>
#include <dma_tags.h>
#include <sio.h>

#include "rom_extract.h"
#include "fontengine.h"
#include "ui.h"
#include "pad.h"
#include "tests/tests.h"

#include "flame_clut.h"
#include "flame_tex.h"

using namespace ui;

page* mainPage = new page(nullptr, "Fobesmark (" GIT_VERSION ")");

page* ui::currentPage = mainPage;

qword_t* drawArea;
const size_t drawAreaQW = 2048;

u32 btnTexAddress = 0;
u32 flameClutAddress = 0;

framebuffer_t internal::fb;
zbuffer_t internal::zb;

int ui::iVsyncSemaID;
void initialiseFrame()
{
	using namespace internal;
	graph_initialize(fb.address, fb.width, fb.height, fb.psm, 0, 0);
}

// Maybe we should set more registers?
void initialiseDrawContext()
{
	using namespace internal;
	qword_t* drawContextPacket = (qword_t*)aligned_alloc(64, sizeof(qword_t) * 20);
	qword_t* q = drawContextPacket;

	PACK_GIFTAG(q, GIF_SET_TAG(15, 1, GIF_PRE_DISABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1),
		GIF_REG_AD);
	q++;
	// FRAME
	PACK_GIFTAG(q, GS_SET_FRAME(fb.address >> 11, fb.width >> 6, fb.psm, fb.mask), GS_REG_FRAME);
	q++;
	// TEST
	PACK_GIFTAG(q, GS_SET_TEST(1, 7, 0, 2, 0, 0, 1, 1), GS_REG_TEST);
	q++;
	// PABE
	PACK_GIFTAG(q, GS_SET_PABE(DRAW_DISABLE), GS_REG_PABE);
	q++;
	PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 0, 1, 128), GS_REG_ALPHA);
	q++;
	// ZBUF
	PACK_GIFTAG(q, GS_SET_ZBUF(zb.address >> 11, zb.zsm, zb.mask), GS_REG_ZBUF);
	q++;
	// XYOFFSET
	PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET);
	q++;
	// DITHERING
	PACK_GIFTAG(q, GS_SET_DTHE(0), GS_REG_DTHE);
	q++;
	// PRMODECONT
	PACK_GIFTAG(q, GS_SET_PRMODECONT(1), GS_REG_PRMODECONT);
	q++;
	// SCISSOR
	PACK_GIFTAG(q, GS_SET_SCISSOR(0, fb.width - 1, 0, fb.height - 1), GS_REG_SCISSOR);
	q++;
	// CLAMP
	PACK_GIFTAG(q, GS_SET_CLAMP(0, 0, 0, 0, 0, 0), GS_REG_CLAMP);
	q++;
	// SCANMSK
	PACK_GIFTAG(q, GS_SET_SCANMSK(0), GS_REG_SCANMSK);
	q++;
	PACK_GIFTAG(q, GS_SET_TEXA(0x80, ALPHA_EXPAND_NORMAL, 0x80), GS_REG_TEXA);
	q++;
	PACK_GIFTAG(q, GS_SET_FBA(ALPHA_CORRECT_RGBA32), GS_REG_FBA);
	q++;
	PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP);
	q++;
	PACK_GIFTAG(q, GS_SET_PABE(DRAW_DISABLE), GS_REG_PABE);
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, drawContextPacket, q - drawContextPacket, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
	free(drawContextPacket);
}

struct romdir_entry
{
	char name[10];
	u16 exit_info_size;
	u32 file_size;
};

// Instead of providing our own button icons, we could just make our life difficult
// and use the ones from ROM0.
void initButtonIcons()
{
	FILE* fTEXIMAGE = fopen("rom0:TEXIMAGE", "rb");
	if (fTEXIMAGE == nullptr)
	{
		printf("Failed to open TEXIMAGE, there will be no button icons!\n");
		return;
	}

	// It's a romdir in the rom!
	// Iterate through TEXIMAGE and find the data we want
	int currOffset = 0x11C; // There's an issue with this romdir iterator, magic offset saves the day
	int texbbttnSize = 0;
	while (1)
	{
		romdir_entry entry;
		fread(&entry, 1, sizeof(romdir_entry), fTEXIMAGE);
		if (entry.name[0] == '\0')
		{
			printf("Couldn't find TEXBBTTN in the TEXIMAGE ROM!\n");
			fclose(fTEXIMAGE);
			return;
		}

		if (std::string(entry.name) == "TEXBBTTN")
		{
			texbbttnSize = entry.file_size;
			break;
		}
		currOffset += entry.file_size;
	}

	fseek(fTEXIMAGE, currOffset, SEEK_SET);
	printf("Offset is %x\n", currOffset);
	// Oh, it's also compressed :)
	u8* btnTexComp = (u8*)malloc(texbbttnSize);
	fread(btnTexComp, 1, texbbttnSize, fTEXIMAGE);
	fclose(fTEXIMAGE);

	// Hopefully 32kb will be enough
	u8* btnTexUnComp = (u8*)aligned_alloc(16, 32 * 1024);
	u32 sizeUnComp = Expand(btnTexComp, btnTexUnComp);
	free(btnTexComp);

	FlushCache(0);

	printf("Uncompressed TEXBBTTN comp: %d, uncomp: %d\n", texbbttnSize, sizeUnComp);

	// Now we have the uncompressed data, we can upload it to GS memory
	if (btnTexAddress == 0)
		btnTexAddress = graph_vram_allocate(64, 64, GS_PSM_32, GRAPH_ALIGN_BLOCK);

	qword_t* q = drawArea;
	q = draw_texture_transfer(q, btnTexUnComp + 20, 64, 64, GS_PSM_32, btnTexAddress, 64);
	q = draw_texture_flush(q);

	dma_channel_send_chain(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	free(btnTexUnComp);
}

void rotateFlameVIF1()
{
	qword_t* q = drawArea;
	DMATAG_CNT(q, 7, 0, 0, 0);
	q++;
	PACK_VIFTAG(q, VIF_CMD_NOP, VIF_CMD_NOP, VIF_CMD_NOP, (VIF_CMD_DIRECT << 24) | 8);
	q++;
	PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_BITBLTBUF(0, 0, 0, flameClutAddress >> 6, 1, GS_PSM_16), GS_REG_BITBLTBUF);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXREG(16, 1), GS_REG_TRXREG);
	q++;
	PACK_GIFTAG(q, GS_SET_TRXDIR(0), GS_REG_TRXDIR);
	q++;
	PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_IMAGE, 0), 0);
	q++;
	DMATAG_REF(q, 2, (uintptr_t)flame_clut, 0, 0, 0);
	q++;

	dma_channel_send_chain(DMA_CHANNEL_VIF1, drawArea, q - drawArea, 0, 0);
	printf("VIF1 kick\n");
	dma_channel_wait(DMA_CHANNEL_VIF1, 0);
	printf("VIF1 complete\n");
	printf("GIF STAT after VIF1 channel wait: %x\n", *(u32*)A_EE_GIF_STAT);
	// Rotate the first 8 colours in the flame_clut
	u16* flameClut = (u16*)flame_clut;
	u16 temp = flameClut[1];
	for (int i = 1; i < 6; i++)
		flameClut[i] = flameClut[i + 1];
	flameClut[6] = temp;
}

void initFlameEffect()
{
	if (flameClutAddress == 0)
		// Issue with graphs vram allocation strategy?
		// If we allocate a CT16 buffer, we end up clobbering it with the ITEX4 buffer??
		flameClutAddress = graph_vram_allocate(16, 1, GS_PSM_32, GRAPH_ALIGN_BLOCK);

	qword_t* q = drawArea;
	q = draw_texture_transfer(q, (u8*)flame_clut, 16, 1, GS_PSM_16, flameClutAddress, 64);
	q = draw_texture_flush(q);
	dma_channel_send_chain(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

	u32 flameTexAddress = graph_vram_allocate(64, 64, GS_PSM_4, GRAPH_ALIGN_BLOCK);

	q = drawArea;
	q = draw_texture_transfer(q, (u8*)flame_itex, 32, 32, GS_PSM_4, flameTexAddress, 64);
	q = draw_texture_flush(q);
	dma_channel_send_chain(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);

u32 frame_cnt = 0;
	while (1)
	{
		q = drawArea;
		PACK_GIFTAG(q, GIF_SET_TAG(7, 1, GIF_PRE_ENABLE, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0), GIF_FLG_PACKED, 1),
			GIF_REG_AD);
		q++;
		PACK_GIFTAG(q, GS_SET_TEXA(0x00, 1, 0x80), GS_REG_NOP);
		q++;
		PACK_GIFTAG(q, GS_SET_TEXCLUT(1, 0, 0), GS_REG_TEXCLUT);
		q++;
		PACK_GIFTAG(q, GS_SET_TEX0(flameTexAddress / 64, 1, GS_PSM_4, 5, 5, 1, 1, flameClutAddress / 64, GS_PSM_16, 1, 0, 1), GS_REG_TEX0);
		q++;
		PACK_GIFTAG(q, GS_SET_UV(0, 0), GS_REG_UV);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
		q++;
		PACK_GIFTAG(q, GS_SET_UV(32 << 4, 32 << 4), GS_REG_UV);
		q++;
		PACK_GIFTAG(q, GS_SET_XYZ(128 << 4, 128 << 4, 0), GS_REG_XYZ2);
		q++;
		dma_channel_send_normal(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		printf("GIF complete\n");
		printf("GIF STAT after GIF channel wait: %x\n", *(u32*)A_EE_GIF_STAT);
		if(frame_cnt % 5 == 0)
			rotateFlameVIF1();
		graph_wait_vsync();
		frame_cnt++;
	}
	SleepThread();
}

void ui::initialise(u32 width, u32 height, u32 psm, u32 zsm)
{
	// Reset the GIF
	*(u32*)A_EE_GIF_CTRL = 1;

	// Initialise GS frame stuff
	using namespace internal;

	fb.address = graph_vram_allocate(width, height, psm, GRAPH_ALIGN_PAGE);
	fb.height = height;
	fb.width = width;
	fb.psm = psm;
	fb.mask = 0;

	zb.address = graph_vram_allocate(width, height, zsm, GRAPH_ALIGN_PAGE);
	zb.enable = 1;
	zb.mask = 0;
	zb.method = ZTEST_METHOD_GREATER;
	zb.zsm = zsm;

	initialiseFrame();

	fontengine_init(psm == GS_PSM_24 && zsm == GS_PSMZ_24, fb.address);

	initialiseDrawContext();

	drawArea = (qword_t*)aligned_alloc(64, sizeof(qword_t) * drawAreaQW);

	initButtonIcons();

	initFlameEffect();
	page* pageVUOverview = new page(mainPage, "VU");
	{
		new function(pageVUOverview, "Small Block VU0", tests::vu::smallBlockVU0);
		new function(pageVUOverview, "Small Block VU1", tests::vu::smallBlockVU1);
		new function(pageVUOverview, "Deadlock", tests::vu::deadlock);
	}
	page* pageEmulation = new page(mainPage, "Emulation Tests");
	{
		new function(pageEmulation, "Run tests", tests::emulationTests);
	}
	page* pageGSTests = new page(mainPage, "GS");
	{
		new function(pageGSTests, "AA1 Test", tests::gs::aa1);
	}

	srand(time(0));
}

color_t cLeft = {GS_SET_RGBAQ(0, 0, 0, 255, 1)};
int lchannel = 1;
int lstep = -1;
color_t cRight = {GS_SET_RGBAQ(0, 0, 0, 255, 1)};
int rchannel = 1;
int rstep = -1;

qword_t* ui::clearFrame(qword_t* q, bool includeButtons)
{
_clearFrame:
	s16 channelValue = (cLeft.rgbaq >> (8 * lchannel)) & 0xFF;
	bool lOverflow = (channelValue == 0x5F && lstep == 1) || (channelValue == 0 && lstep == -1);

	channelValue = (cRight.rgbaq >> (8 * rchannel)) & 0xFF;
	bool rOverflow = (channelValue == 0x5F && rstep == 1) || (channelValue == 0 && rstep == -1);

	if (lOverflow)
	{
		int newChannel = 0;
		int newStep = 1;
		do
		{
			newChannel = rand() % 3;
			newStep = rand() % 2 ? 1 : -1;
		} while (newChannel != lchannel && newStep != lstep);
		lchannel = newChannel;
		lstep = newStep;
		goto _clearFrame;
	}
	if (rOverflow)
	{
		int newChannel = 0;
		int newStep = 1;
		do
		{
			newChannel = rand() % 3;
			newStep = rand() % 2 ? 1 : -1;
		} while (newChannel != rchannel && newStep != rstep);
		rchannel = newChannel;
		rstep = newStep;
		goto _clearFrame;
	}

	cLeft.rgbaq += lstep << (8 * lchannel);

	cRight.rgbaq += rstep << (8 * rchannel);

	color_t cMiddle;
	cMiddle.a = 255;
	cMiddle.r = cLeft.r > cRight.r ?
					((cLeft.r - cRight.r) >> 1) + cRight.r :
					((cRight.r - cLeft.r) >> 1) + cLeft.r;

	cMiddle.g = cLeft.g > cRight.g ?
					((cLeft.g - cRight.g) >> 1) + cRight.g :
					((cRight.g - cLeft.g) >> 1) + cLeft.g;

	cMiddle.b = cLeft.b > cRight.b ?
					((cLeft.b - cRight.b) >> 1) + cRight.b :
					((cRight.b - cLeft.b) >> 1) + cLeft.b;

	PACK_GIFTAG(q, GIF_SET_TAG(9, 1, GIF_PRE_ENABLE, GS_SET_PRIM(GS_PRIM_TRIANGLE_STRIP, 1, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 1),
		GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 0, 1, 0), GS_REG_ALPHA);
	q++;
	PACK_GIFTAG(q, cLeft.rgbaq, GS_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(0 << 4, 0 << 4, 0), GS_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, cMiddle.rgbaq, GS_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(0, internal::fb.height << 4, 0), GS_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, cMiddle.rgbaq, GS_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(internal::fb.width << 4, 0 << 4, 0), GS_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, cRight.rgbaq, GS_REG_RGBAQ);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(internal::fb.width << 4, internal::fb.height << 4, 0), GS_REG_XYZ2);
	q++;

	if (includeButtons)
		q = drawButtons(q);
	return q;
}

// Tried to make it pixel perfect
qword_t* ui::drawButtons(qword_t* q)
{
	if (btnTexAddress == 0)
		return q;

	const int yOffsetX = internal::fb.height - 10;
	const int yOffsetO = internal::fb.height - 42;
	PACK_GIFTAG(q, GIF_SET_TAG(10, 1, GIF_PRE_ENABLE, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0), GIF_FLG_PACKED, 1),
		GIF_REG_AD);
	q++;
	PACK_GIFTAG(q, GS_SET_TEX0(btnTexAddress / 64, 1, GS_PSM_32, 6, 6, 1, 1, 0, 0, 0, 0, 0), GS_REG_TEX0);
	q++;
	PACK_GIFTAG(q, GS_SET_TEX1(0, 0, 0, 0, 0, 0, 0), GS_REG_TEX1);
	q++;
	// X Button
	PACK_GIFTAG(q, GS_SET_UV(8, (31 << 4) + 8), GS_REG_UV);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ((internal::fb.width - 87) << 4, (yOffsetX - 32) << 4, 0), GS_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GS_SET_UV((32 << 4) - 6, (64 << 4) - 6), GS_REG_UV);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(((internal::fb.width - 55) << 4) - 15, (yOffsetX << 4) - 15, 0), GS_REG_XYZ2);
	q++;
	// O Button
	PACK_GIFTAG(q, GS_SET_UV((31 << 4) + 8, (31 << 4) + 8), GS_REG_UV);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ((internal::fb.width - 87) << 4, (yOffsetO - 32) << 4, 0), GS_REG_XYZ2);
	q++;
	PACK_GIFTAG(q, GS_SET_UV((64 << 4) - 6, (64 << 4) - 6), GS_REG_UV);
	q++;
	PACK_GIFTAG(q, GS_SET_XYZ(((internal::fb.width - 55) << 4) - 15, (yOffsetO << 4) - 15, 0), GS_REG_XYZ2);
	q++;

	// fontengine text
	q = fontengine_print_string(q, "Ok", internal::fb.width - 55, yOffsetO - 26, 0, GS_SET_RGBAQ(0x99, 0x99, 0x99, 0x80, 1));
	q = fontengine_print_string(q, "Back", internal::fb.width - 55, yOffsetX - 26, 0, GS_SET_RGBAQ(0x99, 0x99, 0x99, 0x80, 1));
	return q;
}

// brilliant name
u32 left2centerjustified(const std::string& in, u32 x)
{
	const size_t str_cnt = in.size();
	return x - ((str_cnt * FE_WIDTH) / 4);
}

void ui::draw()
{
	s32 y = 10;
	s32 x = left2centerjustified(currentPage->text.c_str(), 320);

	qword_t* q = drawArea;
	q = clearFrame(q);
	q = fontengine_print_string(q, currentPage->text.c_str(), x, y, 0, GS_SET_RGBAQ(0x99, 0x99, 0x99, 0x80, 1));

	y = 50;
	for (size_t i = 0; i < currentPage->items.size(); i++)
	{
		if (currentPage->currentItem == i)
			q = fontengine_print_string(q, currentPage->items[i]->text.c_str(),
				left2centerjustified(currentPage->items[i]->text, 320), y, 0, GS_SET_RGBAQ(0x99, 0x40, 0x40, 0x80, 1));
		else
			q = fontengine_print_string(q, currentPage->items[i]->text.c_str(),
				left2centerjustified(currentPage->items[i]->text, 320), y, 0, GS_SET_RGBAQ(0x99, 0x99, 0x99, 0x80, 1));
		y += 15;
	}

	dma_channel_send_normal(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
	dma_channel_wait(DMA_CHANNEL_GIF, 0);
}

void ui::click()
{
	currentPage->click();
}

// item
item::item(item* parent, std::string text, u32 flags)
	: parent(parent)
	, text(text)
	, flags(flags)
{
	if (parent != nullptr)
	{
		reinterpret_cast<page*>(parent)->items.emplace_back(this);
	}
}

// page
page::page(page* parent, std::string title)
	: item(parent, title, 0)
{
}

void page::pageClick(pad::buttonstate action)
{
	switch (action)
	{
		case pad::buttonstate::UP:
		{
			if (currentItem == 0)
				currentItem = items.size() - 1;
			else
				currentItem--;
		}
		break;
		case pad::buttonstate::DOWN:
		{
			if (currentItem == items.size() - 1)
				currentItem = 0;
			else
				currentItem++;
		}
		break;
		case pad::buttonstate::O:
		{
			items[currentItem]->click();
		}
		break;
		case pad::buttonstate::X:
		{
			if (parent != nullptr) // Make sure we aren't the main page!
				currentPage = reinterpret_cast<page*>(parent);
		}
		break;
		default:
			break;
	}
}

void page::click()
{
	currentPage = this;
}

// function
function::function(page* parent, std::string text, typeof(action) action)
	: item(parent, text, 0)
	, action(action)
{
}

void function::click()
{
	// check flags here


	if (action)
		action();
}
