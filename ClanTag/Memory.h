#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>

#include <map>

#define INRANGE(x,a,b)  (x >= a && x <= b) 
#define getBits( x )    (INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define getByte( x )    (getBits(x[0]) << 4 | getBits(x[1]))


#define SCAN_NORMAL 0x0
#define SCAN_AND_READ 0x1
#define SCAN_AND_SUBTRACT 0x2
#define SCAN_OFFSET SCAN_AND_READ | SCAN_AND_SUBTRACT



namespace Memory
{
	class CMemoryModule
	{
	public:
		explicit CMemoryModule(DWORD base, DWORD size, const std::string& name, const std::string& path)
			:m_dwImageBase(base), m_dwImageSize(size), m_szImageName(name), m_szImagePath(path)
		{

		}

		DWORD GetImageBase() const
		{
			return m_dwImageBase;
		}
		DWORD GetImageSize() const
		{
			return m_dwImageSize;
		}
		const std::string& GetImageName() const
		{
			return m_szImageName;
		}
		const std::string& GetImagePath() const
		{
			return m_szImagePath;
		}

	private:
		DWORD m_dwImageBase;
		DWORD m_dwImageSize;
		std::string m_szImageName;
		std::string m_szImagePath;
	};

	class CProcess
	{
	public:

		bool AttachProcess(const std::string& pName)
		{
			if (pName.empty())
			{
				return false;
			}

			m_dwProcessID = GetProcessIDByName("csgo.exe");
			std::cout << "Process ID: " << m_dwProcessID << std::endl;
			if (!m_dwProcessID) return false;

			m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_dwProcessID);

			if (!m_hProcess)
			{
				return false;
			}

			return EnumerateModules();
		}

		void DetachProcess()
		{
			if (m_hProcess)
			{
				CloseHandle(m_hProcess);
			}
			m_hProcess = NULL;
			m_dwProcessID = NULL;

			ClearLoadedModules();
		}

		void ClearLoadedModules()
		{
			for (const auto& kv : m_loadedModules)
			{
				delete kv.second;
			}
		}

		HANDLE GetProcessHandle() const {
			return m_hProcess;
		}

		template <class cData>
		void WPM(DWORD dwAddress, cData Value)
		{
			WriteProcessMemory(this->m_hProcess, (LPVOID)dwAddress, &Value, sizeof(cData), NULL);
		}

		//WRITE MEMORY - Pointer
		template <class cData>
		void WPM(DWORD dwAddress, char *Offset, cData Value)
		{
			Write<cData>(Read<cData>(dwAddress, Offset, false), Value);
		}
		//READ MEMORY 
		template <class cData>
		cData RPM(DWORD dwAddress)
		{
			cData cRead; //Generic Variable To Store Data
			ReadProcessMemory(this->m_hProcess, (LPVOID)dwAddress, &cRead, sizeof(cData), NULL); //Win API - Reads Data At Specified Location 
			return cRead; //Returns Value At Specified dwAddress
		}

		//READ MEMORY - Pointer
		template <class cData>
		cData RPM(DWORD dwAddress, char *Offset, BOOL Type)
		{
			//Variables
			__int32 iSize = GetLength(Offset) - 1; //Size Of *Array Of Offsets 
			dwAddress = Read<DWORD>(dwAddress); //HEX VAL

												//Loop Through Each Offset & Store Hex Value (Address) In dwTMP
			for (__int32 i = 0; i < iSize; i++)
				dwAddress = Read<DWORD>(dwAddress + Offset[i]);

			if (!Type)
				return dwAddress + Offset[iSize]; //FALSE - Return Address
			else
				return Read<cData>(dwAddress + Offset[iSize]); //TRUE - Return Value
		}

		template <class cData>
		void WriteProtected(DWORD dwAddress, cData val)
		{
			DWORD dwOldProtect;
			VirtualProtectEx(m_hProcess, (LPVOID)dwAddress, sizeof(cData), PAGE_EXECUTE_READWRITE, &dwOldProtect);
			WPM<cData>(dwAddress, val);
			VirtualProtectEx(m_hProcess, (LPVOID)dwAddress, sizeof(cData), dwOldProtect, &dwOldProtect);
		}

		CMemoryModule* GetModule(const std::string& ImageName)
		{
			if (m_loadedModules.empty())
			{
				return nullptr;
			}
			return m_loadedModules[ImageName];
		}

		bool EnumerateModules()
		{
			std::cout << "Enumerating modules!" << std::endl;
			HANDLE hModuleList = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, m_dwProcessID);
			if (!hModuleList) return false;

			MODULEENTRY32 modEntry;
			modEntry.dwSize = sizeof(MODULEENTRY32);

			if (!Module32First(hModuleList, &modEntry))
			{
				std::cout << "Failed to enumerate process modules!" << std::endl;
				modEntry = { NULL };
				CloseHandle(hModuleList);
				return false;
			}
			else
			{
				CMemoryModule* pMemoryModule = nullptr;
				do
				{
					char szFilePath[MAX_PATH] = { '\0' };

					GetModuleFileName(modEntry.hModule, szFilePath, MAX_PATH);

					pMemoryModule = new CMemoryModule((DWORD)modEntry.hModule, (DWORD)modEntry.modBaseSize, modEntry.szModule, szFilePath);

					m_loadedModules.insert(std::pair<std::string, CMemoryModule*>(pMemoryModule->GetImageName(), pMemoryModule));

				} while (Module32Next(hModuleList, &modEntry));
			}

			return true;
		}

		bool CompareBytes(const unsigned char* bytes, const char* pattern)
		{
			for (; *pattern; *pattern != ' ' ? ++bytes : bytes, ++pattern) {
				if (*pattern == ' ' || *pattern == '?')
					continue;
				if (*bytes != getByte(pattern))
					return false;
				++pattern;
			}
			return true;
		}

		uintptr_t FindPattern(CMemoryModule* pModule, const char* pattern, short type, uintptr_t patternOffset, uintptr_t addressOffset, bool subtract = true)
		{
			size_t imgSize = pModule->GetImageSize();
			uintptr_t startAddress = pModule->GetImageBase();

			BYTE* pBytes = new BYTE[imgSize];

			if (!ReadProcessMemory(m_hProcess, (LPVOID)startAddress, pBytes, imgSize, nullptr))
			{
				std::cout << "Failed to dump module bytes from module: " << pModule->GetImageName() << std::endl;
				return 0x0;
			}

			auto max = imgSize - 0x1000;
			for (auto off = 0UL; off < max; ++off)
			{
				if (CompareBytes(pBytes + off, pattern))
				{
					auto add = startAddress + off + patternOffset;
					if (type & SCAN_AND_READ)
						ReadProcessMemory(m_hProcess, (LPVOID)add, &add, sizeof(uintptr_t), nullptr);

					if (type & SCAN_AND_SUBTRACT)
						add -= startAddress;

					delete[] pBytes;
					return add + addressOffset;
				}
			}
			delete[] pBytes;
			return 0x0;
		}

	private:

		DWORD GetProcessIDByName(const std::string& name)
		{
			auto hProcessList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (!hProcessList) return 0x0;

			PROCESSENTRY32 procEntry;
			procEntry.dwSize = sizeof(PROCESSENTRY32);

			if (!Process32First(hProcessList, &procEntry))
			{
				std::cout << "Failed to enumerate process list!" << std::endl;
				procEntry = { NULL };
				CloseHandle(hProcessList);
			}
			else
			{
				do
				{
					if (!strcmp(procEntry.szExeFile, name.c_str()))
					{
						std::cout << "Found process: " << name << " hooking..." << std::endl;
						CloseHandle(hProcessList);
						return procEntry.th32ProcessID;
					}
				} while (Process32Next(hProcessList, &procEntry));
			}

			std::cout << "Failed to find process: " << name << std::endl;

			return 0x0;
		}

		HANDLE m_hProcess;
		DWORD m_dwProcessID;

		std::map<std::string, CMemoryModule*> m_loadedModules;
	};
}


struct CSGO_Globals
{
	Memory::CProcess CSGOProcess;
	DWORD m_dwClientDLL;
	DWORD m_dwEngineDLL;
}; extern CSGO_Globals Globals;