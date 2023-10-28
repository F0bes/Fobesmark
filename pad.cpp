#include "pad.h"
#include <loadfile.h>
#include <kernel.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <graph.h> // Todo: Roll our own wait_vsync (easy, but I'm lazy)
#include <stdio.h>

extern char sio2man_irx[];
extern int size_sio2man_irx;

extern char padman_irx[];
extern int size_padman_irx;

using namespace pad;

char* padBuf[256] __attribute__((aligned(64)));

static void padWait(s32 port)
{
	/* Wait for request to complete. */
	while (padGetReqState(port, 0) != PAD_RSTAT_COMPLETE)
		graph_wait_vsync();

	/* Wait for pad to be stable. */
	while (padGetState(port, 0) != PAD_STATE_STABLE)
		graph_wait_vsync();
}

void pad::init()
{
	s32 ret;
	SifInitRpc(0);

	sbv_patch_enable_lmb();

	if ((ret = SifExecModuleBuffer(sio2man_irx, size_sio2man_irx, 0, NULL, NULL)) < 0)
	{
		printf("Failed to load sio2man module (%d)\n", ret);
		SleepThread();
	}

	printf("ret is %d\n", ret);

	if ((ret = SifExecModuleBuffer(padman_irx, size_padman_irx, 0, NULL, NULL)) < 0)
	{
		printf("Failed to load padman module (%d)\n", ret);
		SleepThread();
	}
	printf("ret is %d\n", ret);

	padInit(0); // EE interpreter w/ cache fails here, is hw a problem too?

	printf("Waiting on a controller connection\n");
	printf("Please use slot 0 :)\n");

	padPortOpen(0, 0, &padBuf);

	u16 padConnected = 0;

	while (!padConnected)
	{
		s32 state = padGetState(0, 0);
		if (state == PAD_STATE_STABLE)
		{
			padSetMainMode(0, 0, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
			padWait(0);

			padConnected = 1;
		}
	}

	printf("Pad connected!\n");
}
struct padButtonStatus buttons;
static u32 paddata;
static u32 old_pad;
static u32 new_pad;
static u32 joy_set = 0;

buttonstate pad::readbuttonstate(void)
{
	u16 ret = padRead(0, 0, &buttons);

	if (ret != 0)
	{
		paddata = 0xffff ^ buttons.btns;

		new_pad = paddata & ~old_pad;
		old_pad = paddata;

		if (((buttons.rjoy_v >= 200 || buttons.ljoy_v >= 200) && !joy_set) || new_pad & PAD_DOWN)
		{
			joy_set = 1;
			return buttonstate::DOWN;
		}

		if (((buttons.rjoy_v <= 20 || buttons.ljoy_v <= 20) && !joy_set) || new_pad & PAD_UP)
		{
			joy_set = 1;
			return buttonstate::UP;
		}

		if (new_pad & PAD_CROSS)
			return buttonstate::X;

		if (new_pad & PAD_CIRCLE)
			return buttonstate::O;

		// This worked the first time, don't trust it!
		if (buttons.rjoy_v > 20 && buttons.rjoy_v < 200 && buttons.ljoy_v > 20 && buttons.ljoy_v < 200)
			joy_set = 0;
	}
	else
	{
		printf("Pad read returns 0. Deciding to SleepThread()\n");
		SleepThread();
	}
	return buttonstate::NONE;
}


bool pad::readButton(buttonstate button)
{
	u16 ret = padRead(0, 0, &buttons);

	paddata = 0xffff ^ buttons.btns;
	new_pad = paddata & ~old_pad;
	old_pad = paddata;

	if (ret != 0)
	{
		if (new_pad & PAD_CROSS && button == buttonstate::X)
			return true;

		if (new_pad & PAD_CIRCLE && button == buttonstate::O)
			return true;
	}
	else
	{
		printf("Pad read returns 0. Deciding to SleepThread()\n");
		SleepThread();
	}

	return 0;
}
