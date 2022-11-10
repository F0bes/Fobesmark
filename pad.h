#pragma once
#include <tamtypes.h>
#include <libpad.h>

namespace pad
{
	enum class buttonstate
	{
		X,
		O,
		DOWN,
		UP,
		NONE,
	};

	void init(void);

	buttonstate readbuttonstate(void);

	bool readButton(buttonstate button);
}; // namespace Pad
