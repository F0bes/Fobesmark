#include "tests.h"
#include "vu/vpucodes.h"
#include "ui/ui.h"
#include "fontengine.h"

#include <kernel.h>
#include <ee_regs.h>
#include <sio.h>

// Writes two instructions to memory, which sets a register
// Flushes the cache
// Writes again to that area, changing the value the register is set to
// Execute that area of code
// Because we didn't flush the second time, the value after execution should
// be the first value we set that was flushed.
static bool isiCacheEmulated()
{

	u32* testFunc = (u32*)aligned_alloc(16, sizeof(u32) * 2);
	// Get cached part of main ram
	u32* testFuncCached = (u32*)((u32)testFunc & ~0x20000000);
	testFuncCached[0] = 0x03E00008; // jr ra
	testFuncCached[1] = 0x34040001; // ori a0, zero, 1

	FlushCache(0); // Flush the cache to memory

	testFuncCached[1] = 0x34040002; // ori a0, zero, 2

	u32 output;
	asm volatile(
		"jalr %1\n"
		"nop\n"
		"or %0, $zero, $a0\n"
		: "=r"(output)
		: "r"((u32)testFuncCached)
		: "$a0");


	free(testFunc);
	return output == 1;
}

static bool isdCacheEmulated()
{
	qword_t* testData = (qword_t*)aligned_alloc(16, sizeof(qword_t));
	// Get cached part of main ram
	qword_t* testDataCached = (qword_t*)((u32)testData & ~0x20000000);

	testDataCached->dw[0] = ~(u64)0;
	testDataCached->dw[1] = 0;

	*R_EE_D9_SADR = 0;
	*R_EE_D9_MADR = (u32)testDataCached;
	*R_EE_D9_QWC = 1;

	FlushCache(0);

	testDataCached->dw[0] = 0;
	testDataCached->dw[1] = ~(u64)0;

	*R_EE_D9_CHCR = 0x100;
	while (*R_EE_D9_CHCR & 0x100)
	{
	};

	// Read back from scratchpad

	qword_t* SPR = (qword_t*)0x70000000;

	free(testData);

	return SPR->dw[0] == ~(u64)0 && SPR->dw[1] == 0;
}

// There are hazards when using Q on VU0, using Q does not stall
// (using WAITQ stalls until Q is ready)
// We can load Q with a value, start a DIV and then use Q immediately
// If there is proper pipelining this would give us our first Q value
//
static bool isCOP2Pipelined()
{
	float num alignas(16) = 2.0f;
	asm __volatile__(
		"QMTC2 %1, $vf1\n"
		"VDIV $Q, $vf1x, $vf1x\n" // Divide 2.0 / 2.0
		"VWAITQ\n" // Wait until Q is ready (1)
		"VDIV $Q, $vf0x, $vf1x\n" // Divide 0 / 2 (latency of 7) and store in Q
		"VMULQ $vf1, $vf1, $Q\n" // Multiply 2 by Q and store in vf1
		"SQC2 $vf1, %0\n"
		: "=m"(num)
		: "r"(num));
	return num == 2.0f;
}

// Between the VIF and DMAC there is a fifo that can hold some qwords
// We send the VIF 4 QW of VIFCode, the first of which will have a set
// interrupt bit
// Because the VIF will be stalled, it will stop processing data
// On hardware, the leftover data that was being sent to the VIF will be
// taken by the FIFO, and the channel will be drained
static bool isVIFFifoEmulated()
{
	DIntr();
	// Reset VIF1
	*R_EE_VIF1_FBRST = 1;

	u32 testVIFData[16] alignas(16); // 4 QW of VIF code

	testVIFData[0] = 1 << 31; // VIFNOP + interrupt bit
	for (int i = 1; i < 16; i++)
	{
		testVIFData[i] = 0; // VIFNOP
	}

	*R_EE_D1_MADR = (u32)testVIFData;
	*R_EE_D1_QWC = 4;

	FlushCache(0);

	*R_EE_D1_CHCR = 0x101;

	nopdelay(); // Don't wait on the channel, the VIF is set to stall

	bool fifoPresent = *R_EE_D1_QWC == 0;

	*R_EE_VIF1_FBRST = 1;
	*R_EE_D1_CHCR = 0;

	EIntr();

	return fifoPresent;
}

// Between the DMAC and GIF there is a little FIFO area
// We will set the GIF to stall (by setting the PSE bit)
// When we send data to the GIF, it will not process anything
// That being said, on hardware this data will go into the FIFO, draining
// the DMA channel
static bool isGIFFifoEmulated()
{
	DIntr();
	// Reset GIF
	*R_EE_GIF_CTRL = 1;

	qword_t testGIFData[4] alignas(16); // 4 QW of GIF data

	for (int i = 0; i < 4; i++)
	{
		testGIFData->dw[1] = (u64)2 << 58; // IMAGE MODE 0 QWORD
	}

	*R_EE_D2_MADR = (u32)testGIFData;
	*R_EE_D2_QWC = 4;

	// Set PSE (Temporary Transfer stop)
	*R_EE_GIF_CTRL = 1 << 3;

	FlushCache(0);

	*R_EE_D2_CHCR = 0x100;

	// todo: make this a function
	// and use it for the VIF test as well
	volatile u32 _wait = 0;
	for (int i = 0; i < 100; i++)
	{
		_wait++;
	};

	bool fifoPresent = *R_EE_D2_QWC == 0;

	*R_EE_GIF_CTRL = 1;
	EIntr();
	return fifoPresent;
}

typedef union
{
	struct
	{
		u32 ix;
		u32 iy;
		u32 iz;
		u32 iw;
	};
	struct
	{
		float x;
		float y;
		float z;
		float w;
	};
} VU_REG ALIGNED(16);

// There an operation error of 1 bit when multiplying 1 by a number
// on the VU
// I've sifted through some numbers and found some inaccuracies on the console
// Here, we are multiplying 1 by 4 different numbers I've found to be problematic
// and then comparing them to known results
static bool isMul1ByxErrorEmulated()
{
	DIntr();
	// Numbers with known issues, tested on a console
	// The numbers in parenthesis () is the hex value interpreted
	// as float by the PS2
	const VU_REG in =
		{
			0x43008000, // 128.5
			0x43788000, // 248.5
			0x3E4CCCCC, // 0.199999988079 (0.2)
			0x3F7FFFFF // .999999940395 (1.0)
		};

	const VU_REG expected =
		{
			0x43007FFF, // 128.499985
			0x43787FFF, // 248.499985
			0x3E4CCCCB, // 0.199999973178 (0.2)
			0x3F7FFFFE // 0.999999880791 (1.0)
		};

	VU_REG one = {0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000};

	VU_REG out;
	asm volatile(
		"lqc2 $vf1, %2\n"
		"lqc2 $vf2, %1\n"
		"vmul $vf3, $vf1, $vf2\n"
		"sqc2 $vf3, %0\n"
		: "=m"(out)
		: "m"(in), "m"(one));

	EIntr();

	if (out.ix != expected.ix)
		return false;
	if (out.iy != expected.iy)
		return false;
	if (out.iz != expected.iz)
		return false;
	if (out.iw != expected.iw)
		return false;

	return true;
}
/* Doesn't work, can't get the console to trigger it
static s32 wdSemaID = 0;
static s32 intWatchdogHandler(s32 cause)
{
	sio_puts("intr hit\n");
	iSignalSema(wdSemaID);
	ExitHandler();

	return 0;
}

inline u32 pollVU0Finish()
{
	u32 value;
	asm(
		"cfc2 %0, $vi29\n"
		: "=r"(value));
	return value;
}

static bool isVU0WatchdogEmulated()
{
	DIntr();

	ee_sema_t wdSema;
	wdSema.option = 0;
	wdSema.init_count = 0;
	wdSema.max_count = 1;
	wdSemaID = CreateSema(&wdSema);


	//DisableIntcHandler(INTC_VU0WD);
	s32 intHandlerID = AddIntcHandler(INTC_VU0WD, intWatchdogHandler, 0);
	//	EnableIntc(INTC_VU0WD);
	EnableIntcHandler(INTC_VU0WD);

	VUINSTR infLoop[50] alignas(16);
	VUINSTR* instr = infLoop;

	for (int i = 0; i < 48; i++)
	{
		// NOP
		instr->HI = 0x2FF;
		// IADDIU VI00, VI00, 0
		instr->LO = (0x8 << 25);

		instr++;
	}
	// NOP
	instr->HI = 0x2FF;
	// JR VI00
	instr->LO = (0x24 << 25);

	instr++;

	// NOP
	instr->HI = 0x2FF;
	// IADDIU VI00, VI00, 0
	instr->LO = (0x8 << 25);

	instr++;

	EIntr();

	tests::vu::uploadMicroProgram(0, &infLoop->_64, &instr->_64, 0);

	asm volatile(
		//"vcallms 0\n"
		"nop\n"
		"nop\n"
		"nop\n"
		//"cfc2.i $t0, $vi1\n"
	);

	sio_puts("Waiting");

	nopdelay();
	nopdelay();
	nopdelay();
	nopdelay();
	nopdelay();
	sio_puts(std::to_string(pollVU0Finish()).c_str());

	WaitSema(wdSemaID);
	sio_puts("Done");
	SleepThread();
	RemoveIntcHandler(INTC_VU0WD, intHandlerID);
	return false;
}

*/

void tests::emulationTests()
{
	qword_t* drawArea = (qword_t*)aligned_alloc(64, sizeof(qword_t) * 2048);
	qword_t* q = drawArea;


	std::string cop2Pipeline = "COP2 Pipeline: ";

	cop2Pipeline.append(isCOP2Pipelined() ? "Present" : "Not Present");

	std::string iCache = "iCache: ";
	iCache.append(isiCacheEmulated() ? "Present" : "Not Present");

	std::string dCache = "dCache: ";
	dCache.append(isdCacheEmulated() ? "Present" : "Not Present");

	std::string vifFifo = "VIF FIFO: ";
	vifFifo.append(isVIFFifoEmulated() ? "Present" : "Not Present");

	std::string gifFifo = "GIF FIFO: ";
	gifFifo.append(isGIFFifoEmulated() ? "Present" : "Not Present");

	std::string vuMulInaccuracy = "VU (1 * ft) Inaccuracy: ";
	vuMulInaccuracy.append(isMul1ByxErrorEmulated() ? "Present" : "Not Present");

	do
	{
		q = drawArea;
		q = ui::clearFrame(q);
		q = fontengine_print_string(q, cop2Pipeline.c_str(), 0, 0, 2, GS_SET_RGBAQ(255, 255, 255, 128, 0));
		q = fontengine_print_string(q, iCache.c_str(), 0, 20, 2, GS_SET_RGBAQ(255, 255, 255, 128, 0));
		q = fontengine_print_string(q, dCache.c_str(), 0, 40, 2, GS_SET_RGBAQ(255, 255, 255, 128, 0));
		q = fontengine_print_string(q, vifFifo.c_str(), 0, 60, 2, GS_SET_RGBAQ(255, 255, 255, 128, 0));
		q = fontengine_print_string(q, gifFifo.c_str(), 0, 80, 2, GS_SET_RGBAQ(255, 255, 255, 128, 0));
		q = fontengine_print_string(q, vuMulInaccuracy.c_str(), 0, 100, 2, GS_SET_RGBAQ(255, 255, 255, 128, 0));


		dma_channel_send_normal(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		WaitSema(ui::iVsyncSemaID);
	} while (pad::readbuttonstate() != pad::buttonstate::X);

	free(drawArea);

	return;
}
