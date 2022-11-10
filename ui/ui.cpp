#include "git.h"

#include <stdio.h>
#include <graph.h>
#include <gs_psm.h>
#include <libpad.h>

#include <ee_regs.h>
#include <sio.h>

#include "fontengine.h"
#include "ui.h"
#include "pad.h"
#include "tests/tests.h"

using namespace ui;

page* mainPage = new page(nullptr, "Fobesmark (" GIT_VERSION ")");

page* ui::currentPage = mainPage;

qword_t* drawArea;
const size_t drawAreaQW = 2048;

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

// TODO: Do page/item init during compile time if possible?
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

	// Initialise fontengine

	drawArea = (qword_t*)aligned_alloc(64, sizeof(qword_t) * drawAreaQW);

	//page* pageEEOverview = new page(mainPage, "EE");
	{
	//	page* overview = new page(pageEEOverview, "Standard");
	//	new function(overview, "Standard function", test);

	//	new page(pageEEOverview, "Extreme");
	}
	page* pageVUOverview = new page(mainPage, "VU");
	{
	//	page* overview = new page(pageVUOverview, "Standard");
		new function(pageVUOverview, "Small Block VU0", tests::vu::smallBlockVU0);
		new function(pageVUOverview, "Small Block VU1", tests::vu::smallBlockVU1);

	//	new page(pageVUOverview, "Extreme");
	}
	page* pageEmulation = new page(mainPage, "Emulation Tests");
	{
		new function(pageEmulation, "Run tests", tests::emulationTests);
	}

	srand(time(0));
}

color_t cLeft = {GS_SET_RGBAQ(0, 0, 0, 255, 1)};
int lchannel = 1;
int lstep = -1;
color_t cRight = {GS_SET_RGBAQ(0, 0, 0, 255, 1)};
int rchannel = 1;
int rstep = -1;

qword_t* ui::clearFrame(qword_t* q)
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

	PACK_GIFTAG(q, GIF_SET_TAG(8, 1, GIF_PRE_ENABLE, GS_SET_PRIM(GS_PRIM_TRIANGLE_STRIP, 1, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 1),
		GIF_REG_AD);
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
	switch(action)
	{
		case pad::buttonstate::UP:
		{
			if(currentItem == 0)
				currentItem = items.size() - 1;
			else
				currentItem--;
		}
		break;
		case pad::buttonstate::DOWN:
		{
			if(currentItem == items.size() - 1)
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
			if(parent != nullptr) // Make sure we aren't the main page!
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


	if(action)
		action();
}
