#pragma once

#include "Utilities/Thread.h"

enum CPUThreadType
{
	CPU_THREAD_PPU,
	CPU_THREAD_SPU,
	CPU_THREAD_RAW_SPU,
	CPU_THREAD_ARMv7,
};

// CPU Thread State Flags
enum : u64
{
	CPU_STATE_STOPPED = (1ull << 0), // basic execution state (stopped by default), removed by Exec()
	CPU_STATE_PAUSED  = (1ull << 1), // pauses thread execution, set by the debugger (manually or after step execution)
	CPU_STATE_SLEEP   = (1ull << 2), // shouldn't affect thread execution, set by Sleep() call, removed by the latest Awake() call, may possibly indicate waiting state of the thread
	CPU_STATE_STEP    = (1ull << 3), // forces the thread to pause after executing just one instruction or something appropriate, set by the debugger
	CPU_STATE_DEAD    = (1ull << 4), // indicates irreversible exit of the thread
	CPU_STATE_RETURN  = (1ull << 5), // used for callback return
	CPU_STATE_SIGNAL  = (1ull << 6),

	CPU_STATE_MAX     = (1ull << 7), // added to (subtracted from) m_state by Sleep()/Awake() calls to trigger status check
};

// "HLE return" exception event
class CPUThreadReturn{};

// CPUThread::Stop exception event
class CPUThreadStop{};

// CPUThread::Exit exception event
class CPUThreadExit{};

class CPUDecoder;

class CPUThread : protected thread_t, public std::enable_shared_from_this<CPUThread>
{
protected:
	atomic_t<u64> m_state; // thread state flags

	std::unique_ptr<CPUDecoder> m_dec;

	const u32 m_id;
	const CPUThreadType m_type;
	const std::string m_name; // changing m_name would be terribly thread-unsafe in current implementation

public:
	using thread_t::mutex;
	using thread_t::cv;

protected:
	CPUThread(CPUThreadType type, const std::string& name, std::function<std::string()> thread_name);

public:
	virtual ~CPUThread() override;

	u32 GetId() const { return m_id; }
	CPUThreadType GetType() const { return m_type; }
	std::string GetName() const { return m_name; }

	bool IsActive() const { return (m_state.load() & CPU_STATE_DEAD) == 0; }
	bool IsStopped() const { return (m_state.load() & CPU_STATE_STOPPED) != 0; }
	virtual bool IsPaused() const;

	virtual void DumpInformation() const;
	virtual u32 GetPC() const = 0;
	virtual u32 GetOffset() const = 0;
	virtual void DoRun() = 0;
	virtual void Task() = 0;

	virtual void InitRegs() = 0;
	virtual void InitStack() = 0;
	virtual void CloseStack() = 0;

	// initialize thread
	void Run();

	// called by the debugger, don't use
	void Pause();

	// called by the debugger, don't use
	void Resume();

	// stop thread execution
	void Stop();

	// start thread execution (removing STOP status)
	void Exec();

	// exit thread execution
	void Exit();

	// called by the debugger, don't use
	void Step();

	// trigger thread status check
	void Sleep();

	// untrigger thread status check
	void Awake();

	// set SIGNAL and notify (returns true if set)
	bool Signal();

	// test SIGNAL and reset
	bool Signaled();

	// process m_state flags, returns true if the checker must return
	bool CheckStatus();

	std::string GetFName() const
	{
		return fmt::format("%s[0x%x] Thread (%s)", GetTypeString(), m_id, m_name);
	}

	static const char* CPUThreadTypeToString(CPUThreadType type)
	{
		switch (type)
		{
		case CPU_THREAD_PPU: return "PPU";
		case CPU_THREAD_SPU: return "SPU";
		case CPU_THREAD_RAW_SPU: return "RawSPU";
		case CPU_THREAD_ARMv7: return "ARMv7";
		}

		return "Unknown";
	}

	const char* ThreadStatusToString()
	{
		// TODO

		//switch (ThreadStatus())
		//{
		//case CPUThread_Ready: return "Ready";
		//case CPUThread_Running: return "Running";
		//case CPUThread_Paused: return "Paused";
		//case CPUThread_Stopped: return "Stopped";
		//case CPUThread_Sleeping: return "Sleeping";
		//case CPUThread_Break: return "Break";
		//case CPUThread_Step: return "Step";
		//}

		return "Unknown";
	}

	const char* GetTypeString() const
	{
		return CPUThreadTypeToString(m_type);
	}

	CPUDecoder* GetDecoder()
	{
		return m_dec.get();
	};

	virtual std::string RegsToString() const = 0;
	virtual std::string ReadRegString(const std::string& reg) const = 0;
	virtual bool WriteRegString(const std::string& reg, std::string value) = 0;

	struct CallStackItem
	{
		u32 pc;
		u32 branch_pc;
	};

	std::vector<CallStackItem> m_call_stack;

	std::string CallStackToString()
	{
		std::string ret = "Call Stack:\n==========\n";

		for(uint i=0; i<m_call_stack.size(); ++i)
		{
			ret += fmt::Format("0x%x -> 0x%x\n", m_call_stack[i].pc, m_call_stack[i].branch_pc);
		}

		return ret;
	}

	void CallStackBranch(u32 pc)
	{
		//look if we're jumping back and if so pop the stack back to that position
		auto res = std::find_if(m_call_stack.rbegin(), m_call_stack.rend(),
			[&pc, this](CallStackItem &it)
			{
				return CallStackGetNextPC(it.pc) == pc;
			});
		if (res != m_call_stack.rend())
		{
			m_call_stack.erase((res + 1).base(), m_call_stack.end());
			return;
		}

		//add a new entry otherwise
		CallStackItem new_item;

		new_item.branch_pc = pc;
		new_item.pc = GetPC();

		m_call_stack.push_back(new_item);
	}

	virtual u32 CallStackGetNextPC(u32 pc)
	{
		return pc + 4;
	}
};

class cpu_thread
{
protected:
	std::shared_ptr<CPUThread> thread;

public:
	//u32 get_entry() const
	//{
	//	return thread->entry;
	//}

	virtual cpu_thread& args(std::initializer_list<std::string> values) = 0;

	virtual cpu_thread& run() = 0;

	//u64 join()
	//{
	//	if (!joinable())
	//		throw EXCEPTION("thread must be joinable for join");

	//	thread->SetJoinable(false);

	//	while (thread->IsRunning())
	//		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // hack

	//	return thread->GetExitStatus();
	//}

	//bool joinable() const
	//{
	//	return thread->IsJoinable();
	//}

	u32 get_id() const
	{
		return thread->GetId();
	}
};
