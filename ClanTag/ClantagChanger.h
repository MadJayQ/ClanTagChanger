#pragma once
#include <string>
#include <algorithm>
#include <vector>

struct ClanTagAnimFrame_t {
	__int64 frametime_ms;
	std::string frame_text;

	explicit ClanTagAnimFrame_t() {}
	explicit ClanTagAnimFrame_t(std::string t, __int64 time)
		: frame_text(t), frametime_ms(time) {
	}
};

struct ClanTagAnimation_t {
	unsigned long current_frame;
	std::vector<ClanTagAnimFrame_t> anim_frames;
	explicit ClanTagAnimation_t() {}
	explicit ClanTagAnimation_t(std::vector<ClanTagAnimFrame_t> frames) : anim_frames(frames) {
		current_frame = 0;
	}

	ClanTagAnimFrame_t GetCurrentFrame() const {
		return anim_frames[current_frame];
	}

	void IncrementFrame() {
		current_frame = (current_frame + 1) % anim_frames.size();
	}
};

namespace Features {
	void ClantagChanger_Init();
	void ClantagChanger_Tick(__int64 current_time);
	void ClantagChanger_SetAnimTime(__int32 anim_time);
	void ClantagChanger_SetAnimMode(int anim_mode);
	void ClantagChanger_SetText(std::string text);
	void ClantagChanger_ForceUpdate();
}
