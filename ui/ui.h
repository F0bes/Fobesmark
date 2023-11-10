#pragma once

#include <tamtypes.h>
#include <gs_gp.h>
#include <draw.h>

#include <vector>
#include <string>
#include <functional>
#include <optional>

#include "pad.h"

namespace ui
{
	enum iopts : u32
	{
		NONE = 0,
		RESET_VU1 = 1,
	};

	struct ui_color
	{
		u16 R, G, B, A;
		ui_color(u16 r, u16 g, u16 b, u16 a)
			: R(r)
			, G(g)
			, B(b)
			, A(a){};
		ui_color()
			: R(0)
			, G(0)
			, B(0)
			, A(0){};
		u64 REG() { return GS_SET_RGBAQ(this->R, this->G, this->B, this->A, 0x0); }
	};


	struct item
	{
		item* parent;
		std::string text;
		u32 flags;

		virtual void click() = 0;

		item(item* parent, std::string text, u32 flags);
	};

	struct page : public item
	{
		size_t currentItem = 0;
		std::vector<item*> items;

		page(page* parent, std::string title);

		void click();
		void pageClick(pad::buttonstate action);
	};

	struct function : public item
	{
		std::function<void()> action;

		void click();
		function(page* parent, std::string text, typeof(action) action);
	};

	extern page* currentPage;
	
	extern int iVsyncSemaID;
	void initialise(u32 width, u32 height, u32 psm, u32 zsm);
	void draw();
	qword_t* clearFrame(qword_t* q, bool includeButtons = true);
	qword_t* drawButtons(qword_t* q);
	void click();

	namespace internal
	{
		extern framebuffer_t fb;
		extern zbuffer_t zb;
	}

} // namespace ui
