#include <stdio.h>
#include <kernel.h>
#include <gs_psm.h>

#include "pad.h"
#include "ui/ui.h"

static unsigned char mainThreadStack[0x3000] ALIGNED(16);

ee_sema_t iVsyncSema;
s32 iVsyncHandler(s32 cause)
{
	iSignalSema(ui::iVsyncSemaID);
	ExitHandler();

	return 1;
}

s32 mainThreadFunc()
{
	ui::initialise(640,448,GS_PSM_24, GS_PSMZ_24);

	pad::init();

	DIntr();
	
	iVsyncSema.init_count = 0;
	iVsyncSema.max_count = 1;
	iVsyncSema.option = 0;
	ui::iVsyncSemaID = CreateSema(&iVsyncSema);

	DisableIntcHandler(INTC_VBLANK_S);
	AddIntcHandler(INTC_VBLANK_S, iVsyncHandler, 0);
	EnableIntc(INTC_VBLANK_S);

	EIntr();

	// mainloop
	while(true)
	{
		ui::draw();
		auto state = pad::readbuttonstate();
		if(state != pad::buttonstate::NONE)
		{
			ui::currentPage->pageClick(state);
		}
		WaitSema(ui::iVsyncSemaID);
	}

	SleepThread();
	return 0;
}

int main()
{
	ee_thread_t mainThread;
	mainThread.initial_priority = 0x40;
	mainThread.gp_reg = &_gp;
	mainThread.stack = mainThreadStack;
	mainThread.stack_size = sizeof(mainThreadStack);
	mainThread.func = (void*)mainThreadFunc;

	s32 mainThreadID = CreateThread(&mainThread);

	StartThread(mainThreadID, nullptr);
	

	SleepThread();
}
