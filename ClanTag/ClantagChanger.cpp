#include "ClantagChanger.h"
#include "Memory.h"

constexpr char character_builders[5] = { 0x7C, 0x2F, 0x2D, 0x2D, 0x5C };
int current_animation_mode = 0;
int current_width = 15;
unsigned __int32 anim_frame_time = 450UL;
std::string clantag_text = "";
std::string last_final_text = "";

static __int64 anim_last_frametime = 0UL;

constexpr int MAX_TEXT_SIZE = 15;

ClanTagAnimation_t current_animation;

static __forceinline void SetClanTag(const char* tag, const char* name)
{
	auto ProcessHandle = Globals.CSGOProcess.GetProcessHandle();

	/*
		51						push	ecx
		52						push	edx
		B9 00 00 00 00			mov		ecx, tag
		BA 00 00 00 00			mov	    edx, name
		E8 00 00 00 00			call	0
		83 04 24 0A				add		dword ptr [esp], 0A
		68 00 00 00 00			push	engine.dll + OFFSET
		C3						ret
		5A						pop		edx
		59						pop		ecx
		C3						ret
		00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
		00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
	*/
	unsigned char Shellcode[] =
		"\x51"                   
		"\x52"                  
		"\xB9\x00\x00\x00\x00"   
		"\xBA\x00\x00\x00\x00"   
		"\xE8\x00\x00\x00\x00"  
		"\x83\x04\x24\x0A"      
		"\x68\x00\x00\x00\x00"  
		"\xC3"          
		"\x5A"              
		"\x59"               
		"\xC3"           
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" 
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

	unsigned int SHELLCODE_SIZE = sizeof(Shellcode) - 0x21;
	unsigned int TAG_SIZE = (strlen(tag) > MAX_TEXT_SIZE) ? MAX_TEXT_SIZE : strlen(tag);
	unsigned int NAME_SIZE = (strlen(name) > MAX_TEXT_SIZE) ? MAX_TEXT_SIZE : strlen(name);
	unsigned int DATA_SIZE = TAG_SIZE + NAME_SIZE + 2;

	LPVOID ShellCodeAddress = VirtualAllocEx(ProcessHandle,
		0,
		SHELLCODE_SIZE + DATA_SIZE,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE
		);

	DWORD dwTagAddress = (DWORD)ShellCodeAddress + SHELLCODE_SIZE;
	DWORD dwNameAddress = (DWORD)ShellCodeAddress + SHELLCODE_SIZE + TAG_SIZE + 1;
	DWORD dwSetClanAddress = Globals.m_dwEngineDLL + 0x9CE30; 

	memcpy(Shellcode + 0x3, &dwTagAddress, sizeof(DWORD));
	memcpy(Shellcode + 0x8, &dwNameAddress, sizeof(DWORD));
	memcpy(Shellcode + 0x16, &dwSetClanAddress, sizeof(DWORD));
	memcpy(Shellcode + SHELLCODE_SIZE, tag, TAG_SIZE);
	memcpy(Shellcode + SHELLCODE_SIZE + TAG_SIZE + 1, name, NAME_SIZE);

	WriteProcessMemory(ProcessHandle, ShellCodeAddress, Shellcode, SHELLCODE_SIZE + DATA_SIZE, 0);

	HANDLE hThread = CreateRemoteThread(ProcessHandle, NULL, NULL, (LPTHREAD_START_ROUTINE)ShellCodeAddress, NULL, NULL, NULL);
	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(ProcessHandle, ShellCodeAddress, 0, MEM_RELEASE);

}

ClanTagAnimation_t BuildScrollingTextAnim(std::string text, int width) {
	char space = ' ';
	std::replace(text.begin(), text.end(), ' ', space);
	std::string anim_crop = std::string(width, space) + text + std::string(width - 1, space);
	std::vector<ClanTagAnimFrame_t> frames;
	for (unsigned long character = 0UL; character < text.length() + width; character++) {
		auto frame = ClanTagAnimFrame_t(anim_crop.substr(character, width + character), anim_frame_time);
		frames.push_back(frame);
	}

	return ClanTagAnimation_t(frames);
}

ClanTagAnimation_t BuildFormingTextAnim(std::string text, int width) {
	char space = ' ';
	std::replace(text.begin(), text.end(), ' ', space);
	std::string current_frame_text = std::string(width, space);
	std::vector<ClanTagAnimFrame_t> frames;
	for (auto character = 0UL; character < text.length(); character++) {
		auto new_frame_text = current_frame_text;
		for (auto i = 0; i < sizeof(character_builders); i++) {
			new_frame_text[character] = character_builders[i];
			auto frame = ClanTagAnimFrame_t(new_frame_text, anim_frame_time);
			frames.push_back(frame);
		}
		new_frame_text[character] = text[character];
		auto frame = ClanTagAnimFrame_t(new_frame_text, anim_frame_time);
		frames.push_back(frame);
		current_frame_text = new_frame_text;
	}
	return ClanTagAnimation_t(frames);
}

std::string BuildFinalText()
{
	std::string ret = "";
	switch (current_animation_mode)
	{
	case 0: ret = clantag_text; break;
	case 1: ret = current_animation.GetCurrentFrame().frame_text; break;
	case 2: ret = current_animation.GetCurrentFrame().frame_text; break;
	}

	return ret;
}

void BuildClantagAnimation()
{
	switch (current_animation_mode)
	{
	case 0: break;
	case 1: current_animation = BuildScrollingTextAnim(clantag_text, current_width); break;
	case 2: current_animation = BuildFormingTextAnim(clantag_text, current_width); break;
	}
}

void Features::ClantagChanger_Init()
{
	clantag_text = "Test";
	BuildClantagAnimation();
}
void Features::ClantagChanger_Tick(__int64 current_time)
{
	//Check client connection
	if (current_animation_mode > 0) {
		auto frame = current_animation.GetCurrentFrame();
		if (current_time - anim_last_frametime > frame.frametime_ms)
		{
			anim_last_frametime = current_time;
			current_animation.IncrementFrame();
			std::string final_text = BuildFinalText();
			SetClanTag(final_text.c_str(), "");
		}
	}
	else
	{
		std::string final_text = BuildFinalText();
		if (last_final_text.compare(final_text))
		{
			last_final_text = final_text;
			SetClanTag(final_text.c_str(), "");
		}
	}
}

void Features::ClantagChanger_SetAnimTime(__int32 anim_time)
{
	anim_frame_time = anim_time;
}

void Features::ClantagChanger_SetAnimMode(int anim_mode)
{
	current_animation_mode = anim_mode;
}

void Features::ClantagChanger_SetText(std::string text)
{
	clantag_text = text;
}

void Features::ClantagChanger_ForceUpdate()
{
	BuildClantagAnimation();
}
