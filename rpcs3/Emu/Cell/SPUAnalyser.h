#pragma once

#include "Utilities/SharedMutex.h"

#include <set>

// SPU Instruction Type
namespace spu_itype
{
	enum type
	{
		UNK = 0,

		STOP,
		LNOP,
		SYNC,
		DSYNC,
		MFSPR,
		RDCH,
		RCHCNT,
		SF,
		OR,
		BG,
		SFH,
		NOR,
		ABSDB,
		ROT,
		ROTM,
		ROTMA,
		SHL,
		ROTH,
		ROTHM,
		ROTMAH,
		SHLH,
		ROTI,
		ROTMI,
		ROTMAI,
		SHLI,
		ROTHI,
		ROTHMI,
		ROTMAHI,
		SHLHI,
		A,
		AND,
		CG,
		AH,
		NAND,
		AVGB,
		MTSPR,
		WRCH,
		BIZ,
		BINZ,
		BIHZ,
		BIHNZ,
		STOPD,
		STQX,
		BI,
		BISL,
		IRET,
		BISLED,
		HBR,
		GB,
		GBH,
		GBB,
		FSM,
		FSMH,
		FSMB,
		FREST,
		FRSQEST,
		LQX,
		ROTQBYBI,
		ROTQMBYBI,
		SHLQBYBI,
		CBX,
		CHX,
		CWX,
		CDX,
		ROTQBI,
		ROTQMBI,
		SHLQBI,
		ROTQBY,
		ROTQMBY,
		SHLQBY,
		ORX,
		CBD,
		CHD,
		CWD,
		CDD,
		ROTQBII,
		ROTQMBII,
		SHLQBII,
		ROTQBYI,
		ROTQMBYI,
		SHLQBYI,
		NOP,
		CGT,
		XOR,
		CGTH,
		EQV,
		CGTB,
		SUMB,
		HGT,
		CLZ,
		XSWD,
		XSHW,
		CNTB,
		XSBH,
		CLGT,
		ANDC,
		FCGT,
		DFCGT,
		FA,
		FS,
		FM,
		CLGTH,
		ORC,
		FCMGT,
		DFCMGT,
		DFA,
		DFS,
		DFM,
		CLGTB,
		HLGT,
		DFMA,
		DFMS,
		DFNMS,
		DFNMA,
		CEQ,
		MPYHHU,
		ADDX,
		SFX,
		CGX,
		BGX,
		MPYHHA,
		MPYHHAU,
		FSCRRD,
		FESD,
		FRDS,
		FSCRWR,
		DFTSV,
		FCEQ,
		DFCEQ,
		MPY,
		MPYH,
		MPYHH,
		MPYS,
		CEQH,
		FCMEQ,
		DFCMEQ,
		MPYU,
		CEQB,
		FI,
		HEQ,
		CFLTS,
		CFLTU,
		CSFLT,
		CUFLT,
		BRZ,
		STQA,
		BRNZ,
		BRHZ,
		BRHNZ,
		STQR,
		BRA,
		LQA,
		BRASL,
		BR,
		FSMBI,
		BRSL,
		LQR,
		IL,
		ILHU,
		ILH,
		IOHL,
		ORI,
		ORHI,
		ORBI,
		SFI,
		SFHI,
		ANDI,
		ANDHI,
		ANDBI,
		AI,
		AHI,
		STQD,
		LQD,
		XORI,
		XORHI,
		XORBI,
		CGTI,
		CGTHI,
		CGTBI,
		HGTI,
		CLGTI,
		CLGTHI,
		CLGTBI,
		HLGTI,
		MPYI,
		MPYUI,
		CEQI,
		CEQHI,
		CEQBI,
		HEQI,
		HBRA,
		HBRR,
		ILA,
		SELB,
		SHUFB,
		MPYA,
		FNMS,
		FMA,
		FMS,
	};

	// Enable address-of operator for spu_decoder<>
	constexpr type operator &(type value)
	{
		return value;
	}
};

class SPUThread;

// SPU basic function information structure
struct spu_function_t
{
	// Entry point (LS address)
	const u32 addr;

	// Function size (in bytes)
	const u32 size;

	// Function contents (binary copy)
	std::vector<be_t<u32>> data;

	// Basic blocks (start addresses)
	std::set<u32> blocks;

	// Functions possibly called by this function (may not be available)
	std::set<u32> adjacent;

	// Jump table values (start addresses)
	std::set<u32> jtable;

	// Whether ila $SP,* instruction found
	bool does_reset_stack;

	// Pointer to the compiled function
	u32(*compiled)(SPUThread* _spu, be_t<u32>* _ls) = nullptr;

	spu_function_t(u32 addr, u32 size)
		: addr(addr)
		, size(size)
	{
	}
};

// SPU Function Database (must be global or PS3 process-local)
class SPUDatabase final
{
	shared_mutex m_mutex;

	// All registered functions (uses addr and first instruction as a key)
	std::unordered_multimap<u64, std::shared_ptr<spu_function_t>> m_db;

	// For internal use
	std::shared_ptr<spu_function_t> find(const be_t<u32>* data, u64 key, u32 max_size);

public:
	SPUDatabase();
	~SPUDatabase();

	// Try to retrieve SPU function information
	std::shared_ptr<spu_function_t> analyse(const be_t<u32>* ls, u32 entry, u32 limit = 0x40000);
};
