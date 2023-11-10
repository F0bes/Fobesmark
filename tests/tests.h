#pragma once

#include <tamtypes.h>

namespace tests
{
	void emulationTests();

	namespace vu
	{
		bool uploadMicroProgram(const u32 offset, const u64* start, const u64* end, u32 vu1);
		void smallBlockVU0();
		void smallBlockVU1();
		void deadlock();
	}

	namespace gs
	{
		void aa1();
	}
} // namespace tests
