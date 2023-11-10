#include "../tests.h"
#include "ui/ui.h"
#include "fontengine.h"

#include <kernel.h>
#include <dma.h>
#include <graph.h>

void tests::gs::aa1()
{
	qword_t* drawArea = (qword_t*)aligned_alloc(64, sizeof(qword_t) * 500);
	const int brW = ui::internal::fb.width << 4;
	const int brH = ui::internal::fb.height << 4;
	qword_t* q = drawArea;
	int offset = 0;
	int v_cnt = 0;
	do
	{
		v_cnt++;
		if(v_cnt == 5)
		{
			offset++;
			v_cnt = 0;
		}

		if(offset == (1 << 8))
			offset = 0;

		q = drawArea;
		PACK_GIFTAG(q, GIF_SET_TAG(127, 1, GIF_PRE_ENABLE, GS_PRIM_SPRITE, GIF_FLG_PACKED, 1),
			GIF_REG_AD);
		q++;
		{ // Screen clear
			PACK_GIFTAG(q, GS_SET_RGBAQ(0, 0, 0, 128, 0), GS_REG_RGBAQ);
			q++;
			PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
			q++;
			PACK_GIFTAG(q, GS_SET_XYZ(brW, brH, 0), GS_REG_XYZ2);
			q++;
		}
		{ // Line strips with AA1 OFF
			PACK_GIFTAG(q, GS_PRIM_TRIANGLE, GS_REG_PRIM);
			q++;
			PACK_GIFTAG(q, GS_SET_RGBAQ(127, 127, 127, 128, 0), GS_REG_RGBAQ);
			q++;
			for (int i = 0; i < 30; i++)
			{
				PACK_GIFTAG(q, GS_SET_XYZ((((i * 25) + offset) << 4), 175 << 4, 0), GS_REG_XYZ2);
				q++;
				PACK_GIFTAG(q, GS_SET_XYZ(((i * 25) + 25) << 4, 200 << 4, 0), GS_REG_XYZ2);
				q++;
			}
		}
		{ // Line strips with AA1 ON
			PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_TRIANGLE, 0, 0, 0, 0, 1, 0, 0, 0), GS_REG_PRIM);
			q++;
			PACK_GIFTAG(q, GS_SET_RGBAQ(255, 255, 255, 128, 0), GS_REG_RGBAQ);
			q++;
			for (int i = 0; i < 30; i++)
			{
				PACK_GIFTAG(q, GS_SET_XYZ(((i * 25) + offset) << 4, 200 << 4, 0), GS_REG_XYZ2);
				q++;
				PACK_GIFTAG(q, GS_SET_XYZ(((i * 25) + 25) << 4, 225 << 4, 0), GS_REG_XYZ2);
				q++;
			}
		}
		q = fontengine_print_string(q, "AA1 OFF", 0, 160, 0, GS_SET_RGBAQ(63, 63, 63, 128, 1));
		q = fontengine_print_string(q, "AA1 ON", 0, 225, 0, GS_SET_RGBAQ(128, 128, 128, 128, 1));
		dma_channel_send_normal(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		WaitSema(ui::iVsyncSemaID);
	} while (pad::readbuttonstate() != pad::buttonstate::X);

	free(drawArea);
}
