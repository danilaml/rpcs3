#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/CB_FUNC.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "sys_interrupt.h"

SysCallBase sys_interrupt("sys_interrupt");

s32 sys_interrupt_tag_destroy(u32 intrtag)
{
	sys_interrupt.Warning("sys_interrupt_tag_destroy(intrtag=0x%x)", intrtag);

	const u32 class_id = intrtag >> 8;

	if (class_id != 0 && class_id != 2)
	{
		return CELL_ESRCH;
	}

	const auto thread = Emu.GetCPU().GetRawSPUThread(intrtag & 0xff);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	auto& tag = class_id ? thread->int2 : thread->int0;

	if (s32 old = tag.assigned.compare_and_swap(0, -1))
	{
		if (old > 0)
		{
			return CELL_EBUSY;
		}

		return CELL_ESRCH;
	}

	return CELL_OK;
}

s32 sys_interrupt_thread_establish(vm::ptr<u32> ih, u32 intrtag, u32 intrthread, u64 arg)
{
	sys_interrupt.Warning("sys_interrupt_thread_establish(ih=*0x%x, intrtag=0x%x, intrthread=0x%x, arg=0x%llx)", ih, intrtag, intrthread, arg);

	const u32 class_id = intrtag >> 8;

	if (class_id != 0 && class_id != 2)
	{
		return CELL_ESRCH;
	}

	const auto thread = Emu.GetCPU().GetRawSPUThread(intrtag & 0xff);

	if (!thread)
	{
		return CELL_ESRCH;
	}

	auto& tag = class_id ? thread->int2 : thread->int0;

	// CELL_ESTAT is not returned (can't detect exact condition)

	const auto it = Emu.GetIdManager().get<PPUThread>(intrthread);

	if (!it)
	{
		return CELL_ESRCH;
	}

	{
		LV2_LOCK;

		if (it->custom_task)
		{
			return CELL_EAGAIN;
		}

		if (s32 res = tag.assigned.atomic_op([](s32& value) -> s32
		{
			if (value < 0)
			{
				return CELL_ESRCH;
			}

			value++;
			return CELL_OK;
		}))
		{
			return res;
		}

		it->custom_task = [thread, &tag, arg](PPUThread& CPU)
		{
			const u32 pc   = CPU.PC;
			const u32 rtoc = CPU.GPR[2];

			std::unique_lock<std::mutex> lock(tag.handler_mutex);

			while (!CPU.IsStopped())
			{
				CHECK_EMU_STATUS;

				// call interrupt handler until int status is clear
				if (tag.stat.load())
				{
					CPU.GPR[3] = arg;
					CPU.FastCall2(pc, rtoc);
				}

				tag.cond.wait_for(lock, std::chrono::milliseconds(1));
			}
		};
	}

	*ih = Emu.GetIdManager().make<lv2_int_handler_t>(it);
	it->Exec();

	return CELL_OK;
}

s32 _sys_interrupt_thread_disestablish(u32 ih, vm::ptr<u64> r13)
{
	sys_interrupt.Todo("_sys_interrupt_thread_disestablish(ih=0x%x, r13=*0x%x)", ih, r13);

	const auto handler = Emu.GetIdManager().get<lv2_int_handler_t>(ih);

	if (!handler)
	{
		return CELL_ESRCH;
	}

	// TODO: wait for sys_interrupt_thread_eoi() and destroy interrupt thread

	*r13 = handler->thread->GPR[13];

	return CELL_OK;
}

void sys_interrupt_thread_eoi(PPUThread& CPU)
{
	sys_interrupt.Log("sys_interrupt_thread_eoi()");

	// TODO: maybe it should actually unwind the stack of PPU thread?

	CPU.GPR[1] = align(CPU.stack_addr + CPU.stack_size, 0x200) - 0x200; // supercrutch to bypass stack check

	CPU.FastStop();
}
