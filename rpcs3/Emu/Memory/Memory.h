#pragma once

#include "MemoryBlock.h"

enum MemoryType
{
	Memory_PS3,
	Memory_PSV,
	Memory_PSP,
};

class MemoryBase
{
	std::vector<MemoryBlock*> MemoryBlocks;

public:
	std::mutex mutex;

	MemoryBlock* UserMemory;

	DynamicMemoryBlock MainMem;
	DynamicMemoryBlock Userspace;
	DynamicMemoryBlock RSXFBMem;
	DynamicMemoryBlock StackMem;
	VirtualMemoryBlock RSXIOMem;

	struct
	{
		DynamicMemoryBlock RAM;
		DynamicMemoryBlock Userspace;
	}
	PSV;

	struct
	{
		DynamicMemoryBlock Scratchpad;
		DynamicMemoryBlock VRAM;
		DynamicMemoryBlock RAM;
		DynamicMemoryBlock Kernel;
		DynamicMemoryBlock Userspace;
	}
	PSP;

	bool m_inited;

	MemoryBase()
	{
		m_inited = false;
	}

	~MemoryBase()
	{
		Close();
	}

	void Init(MemoryType type);

	void Close();

	u32 GetUserMemTotalSize()
	{
		return UserMemory->GetSize();
	}

	u32 GetUserMemAvailSize()
	{
		return UserMemory->GetSize() - UserMemory->GetUsedSize();
	}

	u32 Alloc(const u32 size, const u32 align)
	{
		return UserMemory->AllocAlign(size, align);
	}

	bool Free(const u32 addr)
	{
		return UserMemory->Free(addr);
	}

	bool Map(const u32 addr, const u32 size);

	bool Unmap(const u32 addr);

	MemoryBlock* Get(const u32 addr);
};

extern MemoryBase Memory;

#include "vm.h"
