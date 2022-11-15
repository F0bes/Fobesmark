#include "../tests.h"
#include "vpucodes.h"
#include "ui/ui.h"

#include <tamtypes.h>
#include <ee_regs.h>
#include <kernel.h>
#include <dma.h>

inline bool pollVU0Finish(void)
{
	bool finished;
	asm(
		"cfc2 %0, $vi29\n"
		"andi %0, %0, 1\n"
		: "=r"(finished));
	return !finished;
}
inline void waitVU0Finish(void)
{
	asm(
		"vu0_wait%=:\n"
		"cfc2 $t0, $vi29\n" // VPU-STAT
		"andi $t0,$t0,1\n"
		"bgtz $t0, vu0_wait%=\n" ::
			: "$t0");
}

inline bool pollVU1Finish(void)
{
	bool active;
	asm(
		"li %0, 1\n"
		"bc2t vu1_active_%=\n"
		"nop\n"
		"li %0, 0\n"
		"vu1_active_%=:\n"
		: "=r"(active));
	return active;
}
inline void waitVU1Finish(void)
{
	asm(
		"vu1_active_%=:\n"
		"bc2t vu1_active_%=\n"
		"nop\n"
		);
}

// Return true on success
bool tests::vu::uploadMicroProgram(const u32 offset, const u64* start, const u64* end, u32 vu1)
{
	u32 vp[0xFFF] __attribute__((aligned(128)));
	u32 vpi = 0;

	u32 progInstructions = (end - start);
	if (progInstructions + offset > 512 && !vu1) // overflowed VU0 code memory
		return false;
	// TODO: VU1 code memory overflow check

	u32 mpgBlocks = 0;
	vp[vpi++] = VIFFLUSHE;
	for (u32 instruction = 0; instruction < progInstructions; instruction++)
	{
		if (!(instruction % 256))
		{
			u32 size = progInstructions - (mpgBlocks * 256); // Get the remaining amount of instructions to send
			if (size >= 256) // If the remaining instructions to be sent to VIF is >= 256
				size = 0; // Then send the maximum amount in this mpg (0, but is interpreted as 256)

			// If we are in our first mpg block then set the offset to be zero
			vp[vpi++] = VIFMPG((size + offset), (mpgBlocks == 0 ? 0 : (mpgBlocks * 256)));

			mpgBlocks++;
		}
		vp[vpi++] = start[instruction];
		vp[vpi++] = start[instruction] >> 32;
	}

	vp[vpi++] = VIFMSCAL(0); // Call the micro program, forces recompilation
	vp[vpi++] = VIFFLUSHE;

	while (vpi % 4) // Align the packet
		vp[vpi++] = VIFNOP;

	if (vu1)
	{
		*R_EE_D1_MADR = (u32)vp;
		*R_EE_D1_QWC = vpi / 4;
	}
	else
	{
		*R_EE_D0_MADR = (u32)vp;
		*R_EE_D0_QWC = vpi / 4;
	}

	FlushCache(0);

	if (vu1)
		*R_EE_D1_CHCR = 0x101;
	else
		*R_EE_D0_CHCR = 0x101;

	FlushCache(0);

	while (vu1 ? *R_EE_D1_CHCR == 0x100 : *R_EE_D0_CHCR == 0x100)
	{
	}

	return true;
}

void smallBlocks(const bool VU1)
{
	// Generate a micro program
	// Completely scuffed don't want to do this again

	const u32 BLOCK_COUNT = 100;
	const u32 LASTPC = (BLOCK_COUNT * 4) - 4;
	VUINSTR* mpg = (VUINSTR*)aligned_alloc(64, sizeof(VUINSTR) * (BLOCK_COUNT + 2) * 4);

	mpg[0].HI = 0x2FF;
	// IADDIU VI03, VI00, 0
	mpg[0].LO = (0x8 << 25) | (3 << 16) | 0;
	mpg[1].HI = 0x2FF;
	// IADDIU VI04, VI00, 0x7FF
	mpg[1].LO = (0x8 << 25) | (4 << 16) | 0x7FF | (0xF << 21);
	// NOP
	mpg[2].HI = 0x2FF;
	// IADDIU VI05, VI00, 4
	mpg[2].LO = (0x8 << 25) | (5 << 16) | 4;
	// NOP
	mpg[3].HI = 0x2FF;
	// MOVE VF00, VF00 (nop)
	mpg[3].LO = (0x40 << 25) | (1 << 21) | 0x33C;

	for (u32 i = 4; i < BLOCK_COUNT * 4; i += 4)
	{
		// Set up jump target
		// NOP
		mpg[i].HI = 0x2FF;
		// IADDIU VI01, VI00, i+4
		mpg[i].LO = (0x8 << 25) | (1 << 16) | (i + 4);

		// ADD.x VF[block]x VF[block] VF[block]
		mpg[i + 1].HI = (1 << 21) | (((i / 4) % 32) << 16) | (((i / 4) % 32) << 11) | (((i / 4) % 32) << 6) | 0x28;

		if (i != LASTPC)
		{
			// JALR VI02, VI01
			mpg[i + 1].LO = (0x25 << 25) | (2 << 16) | (1 << 11);
		}
		else
		{
			// IADDIU VI03, VI03, 1
			mpg[i + 1].LO = (0x8 << 25) | (3 << 16) | (3 << 11) | 1;
		}

		// NOP
		mpg[i + 2].HI = 0x2FF;
		// MOVE VF00, VF00 (nop)
		mpg[i + 2].LO = (0x40 << 25) | (1 << 21) | 0x33C;

		// NOP
		mpg[i + 3].HI = 0x2FF;
		// MOVE VF00, VF00 (nop)
		mpg[i + 3].LO = (0x40 << 25) | (1 << 21) | 0x33C;
	}

	u32 lb = (BLOCK_COUNT * 4) + 1;

	// NOP
	mpg[lb].HI = 0x2FF;
	// IBEQ VI03, VI04, 3
	mpg[lb].LO = (0x28 << 25) | (3 << 16) | (4 << 11) | 3;
	lb++;
	// NOP
	mpg[lb].HI = 0x2FF;
	// MOVE VF00, VF00 (nop)
	mpg[lb].LO = (0x40 << 25) | (1 << 21) | 0x33C;
	lb++;

	// NOP
	mpg[lb].HI = 0x2FF;
	// JR VI00
	mpg[lb].LO = (0x24 << 25) | (5 << 11);
	lb++;
	// NOP
	mpg[lb].HI = 0x2FF;
	// MOVE VF00, VF00 (nop)
	mpg[lb].LO = (0x40 << 25) | (1 << 21) | 0x33C;
	lb++;
	// NOP[E]
	mpg[lb].HI = 0x2FF | (1 << 30);
	// MOVE VF00, VF00 (nop)
	mpg[lb].LO = (0x40 << 25) | (1 << 21) | 0x33C;
	lb++;
	// NOP
	mpg[lb].HI = 0x2FF;
	// MOVE VF00, VF00 (nop)
	mpg[lb].LO = (0x40 << 25) | (1 << 21) | 0x33C;


	FlushCache(0);

	qword_t* drawArea = (qword_t*)aligned_alloc(64, sizeof(qword_t) * 100);
	do
	{
		// VU0 and VU1 might take longer than a vsync to finish
		// Instead of waiting for them to finish, just check their status
		// That way we never miss a vsync
		if ((VU1 && pollVU1Finish) || (!VU1 && pollVU0Finish))
			tests::vu::uploadMicroProgram(0, (u64*)mpg, (u64*)(mpg + ((BLOCK_COUNT + 2) * 4)), VU1);

		qword_t* q = drawArea;
		q = ui::clearFrame(q);
		dma_channel_send_normal(DMA_CHANNEL_GIF, drawArea, q - drawArea, 0, 0);
		dma_channel_wait(DMA_CHANNEL_GIF, 0);
		WaitSema(ui::iVsyncSemaID);
	} while (pad::readbuttonstate() != pad::buttonstate::X);

	asm volatile(
		"li $t0, 2\n"
		"ctc2 $t0, $28\n" ::
			: "$t0");

	free(mpg);
	free(drawArea);
}

void tests::vu::smallBlockVU0()
{
	smallBlocks(false);
}

void tests::vu::smallBlockVU1()
{
	smallBlocks(true);
}
