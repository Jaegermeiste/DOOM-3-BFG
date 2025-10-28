/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "win_local.h"

#if defined(_MSC_VER)
#include <intrin.h>       // __cpuidex, _xgetbv
#include <immintrin.h>    // _xgetbv on MSVC
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>        // __cpuid_count
#include <immintrin.h>    // may define _xgetbv on recent GCC/Clang
#include <x86intrin.h>
#endif

#pragma warning(disable:4740)	// warning C4740: flow in or out of inline asm code suppresses global optimization
#pragma warning(disable:4731)	// warning C4731: 'XXX' : frame pointer register 'ebx' modified by inline assembly code

/*
==============================================================

	Clock ticks

==============================================================
*/

/*
================
Sys_GetClockTicks
================
*/
double Sys_GetClockTicks() {
#if defined(ID_WIN64)

	LARGE_INTEGER li = {};

	QueryPerformanceCounter( &li );
	//return (double ) li.LowPart + (double) 0xFFFFFFFF * li.HighPart;
	return static_cast<double>(li.QuadPart);

#else

	unsigned long lo, hi;

	__asm {
		push ebx
		xor eax, eax
		cpuid
		rdtsc
		mov lo, eax
		mov hi, edx
		pop ebx
	}
	return static_cast<double>(lo) + static_cast<double>(0xFFFFFFFF) * hi;

#endif
}

/*
================
Sys_ClockTicksPerSecond
================
*/
double Sys_ClockTicksPerSecond() {
	static double ticks = 0;
#if 0

	if ( !ticks ) {
		LARGE_INTEGER li;
		QueryPerformanceFrequency( &li );
		ticks = li.QuadPart;
	}

#else

	if ( !ticks ) {
		HKEY hKey = {};
		DWORD ProcSpeed = 0;
		DWORD buflen = 0;
		LSTATUS ret = 0;

		if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ, &hKey ) ) {
			buflen = sizeof( ProcSpeed );
			ret = RegQueryValueEx( hKey, "~MHz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&ProcSpeed), &buflen );
			// If we don't succeed, try some other spellings.
			if ( ret != ERROR_SUCCESS ) {
				ret = RegQueryValueEx( hKey, "~Mhz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&ProcSpeed), &buflen );
			}
			if ( ret != ERROR_SUCCESS ) {
				ret = RegQueryValueEx( hKey, "~mhz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&ProcSpeed), &buflen );
			}
			RegCloseKey( hKey );
			if ( ret == ERROR_SUCCESS ) {
				ticks = static_cast<double>(ProcSpeed) * 1000000.0;
			}
		}
	}

#endif
	return ticks;
}


/*
==============================================================

	CPU

==============================================================
*/

/*
================
HasCPUID
================
*/
static bool HasCPUID() {
#if defined(__x86_64__) || defined(_M_X64)
	return true;
#else
#if defined(_MSC_VER) && defined(_M_IX86)
	// MSVC 32-bit: inline asm is available
	__asm 
	{
		pushfd						// save eflags
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		set21				// bit 21 is not set, so jump to set_21
		and		eax, 0xffdfffff		// clear bit 21
		push	eax					// save new value in register
		popfd						// store new value in flags
		pushfd
		pop		eax
		test	eax, 0x00200000		// check ID bit
		jz		good
		jmp		err					// cpuid not supported
set21:
		or		eax, 0x00200000		// set ID bit
		push	eax					// store new value
		popfd						// store new value in EFLAGS
		pushfd
		pop		eax
		test	eax, 0x00200000		// if bit 21 is on
		jnz		good
		jmp		err
	}

err:
	return false;
good:
	return true;
#elif (defined(__GNUC__) || defined(__clang__)) && defined(__i386__)
	uint32 eflags_before, eflags_after;
	__asm__ volatile(
		"pushfl\n\t"
		"popl %0\n\t"
		"movl %0, %1\n\t"
		"xorl $0x200000, %0\n\t"  // toggle ID
		"pushl %0\n\t"
		"popfl\n\t"
		"pushfl\n\t"
		"popl %0\n\t"
		: "=&r"(eflags_after), "=&r"(eflags_before)
		:
		: "cc"
		);
	return ((eflags_after ^ eflags_before) & 0x200000u) != 0;
#else
	// Unknown 32-bit target—assume true on modern compilers/CPUs
	return true;
#endif
#endif

}

enum registers_e {
	_REG_EAX = 0,
	_REG_ECX = 1,
	_REG_EDX = 2,
	_REG_EBX = 3
};

/*
================
CPUID
================
*/
static void CPUID( const uint32 leaf, const uint32 subleaf, uint32 regs[4] ) {
#if 0
	unsigned regEAX, regEBX, regECX, regEDX;

	__asm pusha
	__asm mov eax, func
	__asm __emit 00fh
	__asm __emit 0a2h
	__asm mov regEAX, eax
	__asm mov regEBX, ebx
	__asm mov regECX, ecx
	__asm mov regEDX, edx
	__asm popa

	regs[_REG_EAX] = regEAX;
	regs[_REG_EBX] = regEBX;
	regs[_REG_ECX] = regECX;
	regs[_REG_EDX] = regEDX;
#else
#if defined(_MSC_VER)
	__cpuidex(reinterpret_cast<int*>(regs), static_cast<int>(leaf), static_cast<int>(subleaf));
#elif defined(__GNUC__) || defined(__clang__)
	uint32 regEAX, regEBX, regECX, regEDX;
	__cpuid_count(leaf, subleaf, regEAX, regEBX, regECX, regEDX);
	regs[_REG_EAX] = regEAX;
	regs[_REG_EBX] = regEBX;
	regs[_REG_ECX] = regECX;
	regs[_REG_EDX] = regEDX;
#else
#  error "cpuid: unsupported compiler"
#endif
#endif
}

static inline uint64 xgetbv0() noexcept {
#if defined(_MSC_VER)
	return _xgetbv(0);
#elif defined(__GNUC__) || defined(__clang__)
	// Many libstdc++/libclang setups expose _xgetbv via immintrin/xsaveintrin,
	// but if not, fallback to builtin/asm is fine.
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__GNUC__) && !defined(_xgetbv)
	uint32 eax, edx;
	__asm__ volatile(".byte 0x0f, 0x01, 0xd0"      // xgetbv
		: "=a"(eax), "=d"(edx)
		: "c"(0));
	return (static_cast<uint64>(edx) << 32) | eax;
#else
	return _xgetbv(0);
#endif
#else
	return 0;
#endif
#else
	return 0;
#endif
}

#pragma pack(push,1)
struct SMBIOSHeader { uint8 Type, Length; uint16 Handle; };
#pragma pack(pop)

static inline const char* Sys_GetCpuManufacturerSMBIOS() noexcept {
	static char out[512] = {}; // static storage
	out[0] = '\0';

	DWORD size = GetSystemFirmwareTable('RSMB', 0, nullptr, 0);
	if (!size)
	{
		return nullptr;
	}

	byte* buf = static_cast<byte*>(malloc(size));
	if (!buf)
	{
		return nullptr;
	}

	const char* result = nullptr;
	if (GetSystemFirmwareTable('RSMB', 0, buf, size) == size) {
		byte* p = buf;
		const byte* e = buf + size;
		while (p + sizeof(SMBIOSHeader) <= e) {
			auto* h = reinterpret_cast<const SMBIOSHeader*>(p);
			if (h->Length == 0)
			{
				break;
			}

			// Find end of string-set (double NUL)
			byte* structStart = p;
			byte* next = p + h->Length;
			while (next + 1 < e && (next[0] != 0 || next[1] != 0))
			{
				++next;
			}
			if (next + 1 < e)
			{
				next += 2; // skip double NUL
			}

			if (h->Type == 4 && h->Length >= 0x1A) {
				// Type 4 offset 0x10 = Manufacturer (string index)
				const uint8 idx = structStart[0x10];
				if (idx != 0) {
					const char* s = reinterpret_cast<const char*>(structStart + h->Length);
					// Walk string list (1-based)
					for (uint8 i = 1; *s; ) {
						const size_t L = std::strlen(s);
						if (i == idx) {
							size_t n = (L < sizeof(out) - 1) ? L : sizeof(out) - 1;
							memcpy(out, s, n);
							out[n] = '\0';
							result = out;
							break;
						}
						s += L + 1; ++i;
					}
				}
				if (result)
				{
					break;
				}
			}
			p = next;
		}
	}
	free(buf);
	return result && out[0] ? out : nullptr;
}

static inline const char* GetVendorString() noexcept {
#if defined(ID_CPU_ARCH_X86) || defined(ID_CPU_ARCH_X64)
	static char vendor[13] = {};
	static bool initialized = false;

	if (!initialized) {
		uint32 eax = 0, ebx = 0, ecx = 0, edx = 0;

#if defined(_MSC_VER)
		uint32 regs[4] = {};
		__cpuidex(reinterpret_cast<int*>(regs), 0, 0);
		eax = regs[0]; ebx = regs[1]; ecx = regs[2]; edx = regs[3];
#elif defined(__GNUC__) || defined(__clang__)
		__cpuid_count(0, 0, eax, ebx, ecx, edx);
#endif

		memcpy(&vendor[0], &ebx, 4);
		memcpy(&vendor[4], &edx, 4);
		memcpy(&vendor[8], &ecx, 4);
		vendor[12] = '\0';
		initialized = true;
	}

	return vendor;
#else
	// Non-x86 architecture — CPUID not supported.
	return Sys_GetCpuManufacturerSMBIOS();
#endif
}

/*
================
IsIntel
================
*/
static inline bool IsIntel() noexcept {
#if defined(ID_CPU_ARCH_X86) || defined(ID_CPU_ARCH_X64)
	const char* vendor = GetVendorString();
	return vendor && strcmp(vendor, "GenuineIntel") == 0;
#elif defined(ID_CPU_ARCH_ARM32) || defined(ID_CPU_ARCH_ARM64)
	return false;
#else
	return false;
#endif
}

/*
================
IsAMD
================
*/
static inline bool IsAMD() {
#if 0
	char pstring[16] = {};
	char processorString[13] = {};

	// get name of processor
	CPUID( 0, 0, reinterpret_cast<uint32*>(pstring) );
	processorString[0] = pstring[4];
	processorString[1] = pstring[5];
	processorString[2] = pstring[6];
	processorString[3] = pstring[7];
	processorString[4] = pstring[12];
	processorString[5] = pstring[13];
	processorString[6] = pstring[14];
	processorString[7] = pstring[15];
	processorString[8] = pstring[8];
	processorString[9] = pstring[9];
	processorString[10] = pstring[10];
	processorString[11] = pstring[11];
	processorString[12] = 0;

	if ( strcmp( processorString, "AuthenticAMD" ) == 0 ) {
		return true;
	}
	return false;
#else
#if defined(ID_CPU_ARCH_X86) || defined(ID_CPU_ARCH_X64)
	const char* vendor = GetVendorString();
	return vendor && (strcmp(vendor, "AuthenticAMD") == 0 || strcmp(vendor, "HygonGenuine") == 0);
#elif defined(ID_CPU_ARCH_ARM32) || defined(ID_CPU_ARCH_ARM64)
	return false;
#else
	return false;
#endif
#endif
}

/*
================
IsARM
================
*/
static inline bool IsARM() noexcept {
#if defined(ID_CPU_ARCH_ARM32) || defined(ID_CPU_ARCH_ARM64)
	return true;
#else
	return false;
#endif
}

/*
================
HasCMOV
================
*/
static bool HasCMOV() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID( 1, 0, regs );

	// bit 15 of EDX denotes CMOV existence
	if ( regs[_REG_EDX] & ( 1 << 15 ) ) {
		return true;
	}
	return false;
}

/*
================
Has3DNow
================
*/
static bool Has3DNow() {
	uint32 regs[4] = {};

	// check AMD-specific functions
	CPUID( 0x80000000, 0, regs );
	if ( regs[_REG_EAX] < 0x80000000 ) {
		return false;
	}

	// bit 31 of EDX denotes 3DNow! support
	CPUID( 0x80000001, 0, regs );
	if ( regs[_REG_EDX] & ( 1 << 31 ) ) {
		return true;
	}

	return false;
}

/*
================
HasMMX
================
*/
static bool HasMMX() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID( 1, 0, regs );

	// bit 23 of EDX denotes MMX existence
	if ( regs[_REG_EDX] & ( 1 << 23 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSE
================
*/
static bool HasSSE() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID( 1, 0, regs );

	// bit 25 of EDX denotes SSE existence
	if ( regs[_REG_EDX] & ( 1 << 25 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSE2
================
*/
static bool HasSSE2() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID( 1, 0, regs );

	// bit 26 of EDX denotes SSE2 existence
	if ( regs[_REG_EDX] & ( 1 << 26 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSE3
================
*/
static bool HasSSE3() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID( 1, 0, regs );

	// bit 0 of ECX denotes SSE3 existence
	if ( regs[_REG_ECX] & ( 1 << 0 ) ) {
		return true;
	}
	return false;
}

/*
================
HasSSSE3
================
*/
static bool HasSSSE3() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID(1, 0, regs);

	// bit 9 of ECX denotes SSSE3 existence
	if (regs[_REG_ECX] & (1 << 9)) {
		return true;
	}
	return false;
}

/*
================
HasSSE41
================
*/
static bool HasSSE41() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID(1, 0, regs);

	// bit 19 of ECX denotes SSE4.1 existence
	if (regs[_REG_ECX] & (1 << 19)) {
		return true;
	}
	return false;
}

/*
================
HasSSE42
================
*/
static bool HasSSE42() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID(1, 0, regs);

	// bit 20 of ECX denotes SSE4.1 existence
	if (regs[_REG_ECX] & (1 << 20)) {
		return true;
	}
	return false;
}

/*
================
HasAVX
================
*/
static bool HasAVX() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID(1, 0, regs);

	// bit 28 of ECX denotes AVX existence, xsave (bit 26) and osxsave (bit 27) also required
	const bool has_xsave   = (regs[_REG_ECX] & (1u << 26)) != 0;
	const bool has_osxsave = (regs[_REG_ECX] & (1u << 27)) != 0;
	const bool has_avx_hw  = (regs[_REG_ECX] & (1u << 28)) != 0;

	// OS must enable XMM (bit1) and YMM (bit2) in XCR0 for AVX/AVX2
	uint64 xcr0 = 0;
	if (has_xsave && has_osxsave)
	{
		xcr0 = xgetbv0();
	}

	const bool os_avx_ok = ((xcr0 & 0x6u) == 0x6u); // SSE(1) + YMM(2)

	if (has_avx_hw && os_avx_ok) {
		return true;
	}
	return false;
}

/*
================
HasAVX2
================
*/
static bool HasAVX2() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID(1, 0, regs);

	// xsave (bit 26) and osxsave (bit 27) required
	const bool has_xsave = (regs[_REG_ECX] & (1u << 26)) != 0;
	const bool has_osxsave = (regs[_REG_ECX] & (1u << 27)) != 0;

	memset(&regs, 0, sizeof(regs));

	// AVX2 on page 7
	CPUID(7, 0, regs);

	const bool has_avx2_hw = (regs[_REG_EBX] & (1u << 5)) != 0;

	// OS must enable XMM (bit1) and YMM (bit2) in XCR0 for AVX/AVX2
	uint64 xcr0 = 0;
	if (has_xsave && has_osxsave)
	{
		xcr0 = xgetbv0();
	}

	const bool os_avx_ok = ((xcr0 & 0x6u) == 0x6u); // SSE(1) + YMM(2)

	if (has_avx2_hw && os_avx_ok) {
		return true;
	}
	return false;
}

/*
================
HasFMA3
================
*/
static bool HasFMA3() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID(1, 0, regs);

	// FMA3 = ECX bit 12
	const bool has_fma3_hw = (regs[_REG_ECX] & (1u << 12)) != 0;

	const bool os_avx_ok = HasAVX();

	if (has_fma3_hw && os_avx_ok) {
		return true;
	}
	return false;
}

/*
================
HasAVX512F
================
*/
static bool HasAVX512F() {
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID(1, 0, regs);

	// xsave (bit 26) and osxsave (bit 27) required
	const bool has_xsave = (regs[_REG_ECX] & (1u << 26)) != 0;
	const bool has_osxsave = (regs[_REG_ECX] & (1u << 27)) != 0;

	// OS must enable XMM (bit1) and YMM (bit2) in XCR0 for AVX/AVX2
	uint64 xcr0 = 0;
	if (has_xsave && has_osxsave)
	{
		xcr0 = xgetbv0();
	}

	const bool os_avx_ok = ((xcr0 & 0x6u) == 0x6u); // SSE(1) + YMM(2)
	const bool os_avx512_ok = ((xcr0 & 0xE0u) == 0xE0u) && os_avx_ok; // AVX-512 requires XCR0 enabling of opmask(5), ZMM_hi256(6), hi16_ZMM(7)

	memset(&regs, 0, sizeof(regs));

	// AVX2 on page 7
	CPUID(7, 0, regs);

	if (os_avx512_ok && os_avx_ok) {
		const bool avx512f = (regs[_REG_EBX] & (1u << 16)) != 0;
		return avx512f;
	}
	return false;
}

/*
================
HasNEON
================
*/

#ifndef PF_MAXIMUM_PROCESSORS
#define PF_MAXIMUM_PROCESSORS 0
#endif // PF_MAXIMUM_PROCESSORS

static inline bool HasNEON()
{
	// These PF_* constants exist in recent Windows SDKs; guard with ifdefs.
#ifdef PF_ARM_NEON_INSTRUCTIONS_AVAILABLE
	return IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE) != 0;
#else
	return false;
#endif // PF_ARM_NEON_INSTRUCTIONS_AVAILABLE
}

/*
================
HasSVE
================
*/
static inline bool HasSVE()
{
	// These PF_* constants exist in recent Windows SDKs; guard with ifdefs.
#ifdef PF_ARM_SVE_INSTRUCTIONS_AVAILABLE
	return IsProcessorFeaturePresent(PF_ARM_SVE_INSTRUCTIONS_AVAILABLE) != 0;
#else
	return false;
#endif // PF_ARM_SVE_INSTRUCTIONS_AVAILABLE
}

/*
================
HasSVE2
================
*/
static inline bool HasSVE2()
{
	// These PF_* constants exist in recent Windows SDKs; guard with ifdefs.
#ifdef PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE
	return IsProcessorFeaturePresent(PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE) != 0;
#else
	return false;
#endif // PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE
}

/*
================
HasSVE21
================
*/
static inline bool HasSVE21()
{
	// These PF_* constants exist in recent Windows SDKs; guard with ifdefs.
#ifdef PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE
	return IsProcessorFeaturePresent(PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE) != 0;
#else
	return false;
#endif // PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE
}

/*
================
LogicalProcPerPhysicalProc
================
*/

static size_t LogicalProcPerPhysicalProc() {
#if 0
#define NUM_LOGICAL_BITS   0x00FF0000     // EBX[23:16] Bit 16-23 in ebx contains the number of logical
	// processors per physical processor when execute cpuid with 
	// eax set to 1

	uint32 regebx = 0;
	__asm {
		mov eax, 1
		cpuid
		mov regebx, ebx
	}
	return static_cast<size_t>((regebx & NUM_LOGICAL_BITS) >> 16);
#else
	DWORD bytes = 0;
	if (GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bytes) ||
		GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		return 0;
	}

	byte* buf = static_cast<byte*>(malloc(bytes));
	if (!buf)
	{
		return 0;
	}
	memset(buf, 0, bytes * sizeof(byte));

	size_t result = 0;

	if (GetLogicalProcessorInformationEx(
		RelationProcessorCore,
		reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buf), &bytes)) {

		auto* ex = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buf);
		if (ex && ex->Relationship == RelationProcessorCore) {
			const PROCESSOR_RELATIONSHIP& pr = ex->Processor;

			auto popmask = [](const KAFFINITY m) -> size_t {
				// KAFFINITY is ULONG_PTR; popcount the (up to) 64-bit mask per group
				return static_cast<size_t>(std::popcount(static_cast<unsigned long long>(m)));
				};

			size_t threadsPerCore = 0;
			for (WORD i = 0; i < pr.GroupCount; ++i)
			{
				threadsPerCore += popmask(pr.GroupMask[i].Mask);
			}

			result = threadsPerCore; // 1 (no SMT), 2 (HT), etc.
		}
	}

	free(buf);

	return result; // 0 on failure
#endif
}

/*
================
GetAPIC_ID
================
*/
constexpr auto INITIAL_APIC_ID_BITS = 0xFF000000;  // EBX[31:24] Bits 24-31 (8 bits) return the 8-bit unique
                                          // initial APIC ID for the processor this code is running on.
                                          // Default value = 0xff if HT is not supported
static uint32 GetAPIC_ID() {
#if 0
	unsigned int regebx = 0;
	__asm {
		mov eax, 1
		cpuid
		mov regebx, ebx
	}
	return static_cast<uint32>((regebx & INITIAL_APIC_ID_BITS) >> 24);
#else
	uint32 regs[4] = {};

	// Check highest basic leaf
	CPUID(0, 0, regs);
	const uint32 maxBasic = regs[_REG_EAX];

	memset(&regs, 0, sizeof(regs));

	// Prefer CPUID.1F (Intel/AMD new topology), then CPUID.0B (older Intel)
	if (maxBasic >= 0x1F) {
		CPUID(0x1F, 0, regs);                 // subleaf 0 = SMT level
		if ((regs[_REG_EBX] & 0xFFFF) != 0)
		{
			return regs[_REG_EDX]; // EDX = x2APIC ID
		}
	}
	if (maxBasic >= 0x0B) {
		CPUID(0x0B, 0, regs);
		if ((regs[_REG_EBX] & 0xFFFF) != 0)
		{
			return regs[_REG_EDX]; // EDX = x2APIC ID
		}
	}

	// Legacy initial APIC ID (8-bit) in CPUID.01h:EBX[31:24]
	CPUID(1, 0, regs);
	return (regs[_REG_EBX] >> 24) & 0xFFu;
#endif
}

/*
================
CPUCount

	logicalNum is the number of logical CPU per physical CPU
    physicalNum is the total number of physical processor
	returns one of the HT_* flags
================
*/
enum smtStatus_e : uint8 {
	SMT_NOT_CAPABLE = 0,
	SMT_ENABLED = 1,
	SMT_DISABLED = 2,
	SMT_SUPPORTED_NOT_ENABLED = 3,
	SMT_CANNOT_DETECT = 4,
	MAX_SMT_STATUS
};

static smtStatus_e CPUCount( size_t &logicalNum, size_t &physicalNum ) {
	smtStatus_e statusFlag = SMT_NOT_CAPABLE;
	SYSTEM_INFO info = {};

	physicalNum = 1;
	logicalNum = 1;

	info.dwNumberOfProcessors = 0;
	GetSystemInfo (&info);

	// Number of physical processors in a non-Intel system
	// or in a 32-bit Intel system with Hyper-Threading technology disabled
	physicalNum = info.dwNumberOfProcessors;  

	bool SMT_Enabled = false;

	logicalNum = LogicalProcPerPhysicalProc();

	if ( logicalNum >= 1 ) {	// > 1 doesn't mean HT is enabled in the BIOS
		DWORD  dwProcessAffinity = 0;
		DWORD  dwSystemAffinity = 0;

		// Calculate the appropriate shifts and mask based on the 
		// number of logical processors.

		size_t  i = 1;
		unsigned char PHY_ID_MASK  = 0xFF, PHY_ID_SHIFT = 0;

		while( i < logicalNum ) {
			i *= 2;
 			PHY_ID_MASK  <<= 1;
			PHY_ID_SHIFT++;
		}
		
		HANDLE hCurrentProcessHandle = GetCurrentProcess();
		GetProcessAffinityMask( hCurrentProcessHandle, reinterpret_cast<PDWORD_PTR>(&dwProcessAffinity), reinterpret_cast<PDWORD_PTR>(&dwSystemAffinity) );

		// Check if available process affinity mask is equal to the
		// available system affinity mask
		if ( dwProcessAffinity != dwSystemAffinity ) {
			statusFlag = SMT_CANNOT_DETECT;
			physicalNum = -1;
			return statusFlag;
		}

		DWORD dwAffinityMask = 1;
		while ( dwAffinityMask != 0 && dwAffinityMask <= dwProcessAffinity ) {
			// Check if this CPU is available
			if ( dwAffinityMask & dwProcessAffinity ) {
				if ( SetProcessAffinityMask( hCurrentProcessHandle, dwAffinityMask ) ) {
					Sleep( 0 ); // Give OS time to switch CPU

#if defined(ID_CPU_ARCH_X86) || defined(ID_CPU_ARCH_X64)
					auto APIC_ID = GetAPIC_ID();
					auto LOG_ID  = APIC_ID & ~PHY_ID_MASK;
					auto PHY_ID  = APIC_ID >> PHY_ID_SHIFT;

					if ( LOG_ID != 0 ) {
						SMT_Enabled = true;
					}
#elif defined(ID_CPU_ARCH_ARM) || defined(ID_CPU_ARCH_ARM64)
					// On ARM, we cannot detect per-core IDs easily.
					// Assume HT is enabled if logical > physical.
					if (logicalNum > 1) {
						HT_Enabled = 1;
					}
#endif
				}
			}
			dwAffinityMask = dwAffinityMask << 1;
		}
	        
		// Reset the processor affinity
		SetProcessAffinityMask( hCurrentProcessHandle, dwProcessAffinity );
	    
		if ( logicalNum == 1 ) {  // Normal P4 : HT is disabled in hardware
			statusFlag = SMT_DISABLED;
		} else {
			if ( SMT_Enabled ) {
				// Total physical processors in a Hyper-Threading enabled system.
				physicalNum /= logicalNum;
				statusFlag = SMT_ENABLED;
			} else {
				statusFlag = SMT_SUPPORTED_NOT_ENABLED;
			}
		}
	}
	return statusFlag;
}

/*
================
HasSMT
================
*/
static bool HasSMT() {
	uint32 regs[4] = {};
	bool smt_capable = false;
	bool smt_enabled = false;

#if defined(ID_CPU_ARCH_X86) || defined(ID_CPU_ARCH_X64)
	CPUID(0, 0, regs);
	const auto maxBasic = regs[_REG_EAX];

	auto smt_from_topology = [&](const uint32 leaf)->bool {
		memset(&regs, 0, sizeof(regs));
		CPUID(leaf, 0, regs);
		const uint32 levelType = (regs[_REG_ECX] >> 8) & 0xFF;   // 1 = SMT, 2 = Core
		const uint32 lpAtLevel = regs[_REG_EBX] & 0xFFFF;
		return (levelType == 1) && (lpAtLevel > 1);
		};

	if (maxBasic >= 0x1F && smt_from_topology(0x1F))
	{
		smt_capable = true;
	}
	if (maxBasic >= 0x0B && smt_from_topology(0x0B))
	{
		smt_capable = true;
	}

	if (!smt_capable)
	{
		// get CPU feature bits
		CPUID(1, 0, regs);

		// Legacy fallback: HTT bit + LP-per-package vs cores-per-package
		// CPUID.1: EDX[28] = HTT “Hyper-Threading Technology”
		if (regs[_REG_EDX] & (1 << 28)) {
			smt_capable = true;
		}
	}
#elif defined(ID_CPU_ARCH_ARM) || defined(ID_CPU_ARCH_ARM64)
	// Assume capable
	smt_capable = true;
#endif

	if (smt_capable)
	{
		size_t logicalNum = 0;
		size_t physicalNum = 0;
		auto HTStatusFlag = CPUCount(logicalNum, physicalNum);
		if (HTStatusFlag != SMT_ENABLED) {
			smt_enabled = false;
		}
	}

	if ( smt_enabled ) {
		return true;
	}

	return false;
}

/*
================
HasSMT
================
*/
static bool HasDAZ() {
	__declspec(align(16)) unsigned char FXSaveArea[512] = {};
	unsigned char *FXArea = FXSaveArea;
	DWORD dwMask = 0;
	uint32 regs[4] = {};

	// get CPU feature bits
	CPUID( 1, 0, regs );

	// bit 24 of EDX denotes support for FXSAVE
	if ( !( regs[_REG_EDX] & ( 1 << 24 ) ) ) {
		return false;
	}

	memset( FXArea, 0, sizeof( FXSaveArea ) );

	__asm {
		mov		eax, FXArea
		FXSAVE	[eax]
	}

	dwMask = *reinterpret_cast<DWORD*>(&FXArea[28]);						// Read the MXCSR Mask
	return ( ( dwMask & ( 1 << 6 ) ) == ( 1 << 6 ) );	// Return if the DAZ bit is set
}

/*
================================================================================================

	CPU

================================================================================================
*/

/*
========================
CountSetBits 
Helper function to count set bits in the processor mask.
========================
*/
static DWORD CountSetBits(const ULONG_PTR bitMask ) {
	DWORD LSHIFT = sizeof( ULONG_PTR ) * 8 - 1;
	DWORD bitSetCount = 0;
	ULONG_PTR bitTest = static_cast<ULONG_PTR>(1) << LSHIFT;    

	for ( DWORD i = 0; i <= LSHIFT; i++ ) {
		bitSetCount += ( ( bitMask & bitTest ) ? 1 : 0 );
		bitTest /= 2;
	}

	return bitSetCount;
}

typedef BOOL (WINAPI *LPFN_GLPI)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD );

enum LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL {
    localRelationProcessorCore,
    localRelationNumaNode,
    localRelationCache,
	localRelationProcessorPackage
};

struct cpuInfo_t {
	size_t processorPackageCount;
	size_t processorCoreCount;
	size_t logicalProcessorCount;
	size_t numaNodeCount;
	struct cacheInfo_t {
		int count;
		int associativity;
		int lineSize;
		int size;
	} cacheLevel[3];
};

/*
========================
GetCPUInfo
========================
*/
static bool GetCPUInfo( cpuInfo_t & cpuInfo ) {
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = nullptr;
	PCACHE_DESCRIPTOR Cache;
	LPFN_GLPI	glpi;
	BOOL		done = FALSE;
	DWORD		returnLength = 0;
	DWORD		byteOffset = 0;

	memset( & cpuInfo, 0, sizeof( cpuInfo ) );

	glpi = reinterpret_cast<LPFN_GLPI>(GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation"));
	if (nullptr == glpi ) {
		idLib::Printf( "\nGetLogicalProcessorInformation is not supported.\n" );
		return 0;
	}

	while ( !done ) {
		DWORD rc = glpi( buffer, &returnLength );

		if ( FALSE == rc ) {
			if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
				if ( buffer ) {
					free( buffer );
				}

				buffer = static_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(malloc(returnLength));
			} else {
				idLib::Printf( "Sys_CPUCount error: %d\n", GetLastError() );
				return false;
			}
		} else {
			done = TRUE;
		}
	}

	ptr = buffer;

	while ( byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength ) {
		switch ( static_cast<LOGICAL_PROCESSOR_RELATIONSHIP_LOCAL>(ptr->Relationship) ) {
			case localRelationProcessorCore:
				cpuInfo.processorCoreCount++;

				// A hyperthreaded core supplies more than one logical processor.
				cpuInfo.logicalProcessorCount += CountSetBits( ptr->ProcessorMask );
				break;

			case localRelationNumaNode:
				// Non-NUMA systems report a single record of this type.
				cpuInfo.numaNodeCount++;
				break;

			case localRelationCache:
				// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
				Cache = &ptr->Cache;
				if ( Cache->Level >= 1 && Cache->Level <= 3 ) {
					int level = Cache->Level - 1;
					if ( cpuInfo.cacheLevel[level].count > 0 ) {
						cpuInfo.cacheLevel[level].count++;
					} else {
						cpuInfo.cacheLevel[level].associativity = Cache->Associativity;
						cpuInfo.cacheLevel[level].lineSize = Cache->LineSize;
						cpuInfo.cacheLevel[level].size = Cache->Size;
					}
				}
				break;

			case localRelationProcessorPackage:
				// Logical processors share a physical package.
				cpuInfo.processorPackageCount++;
				break;

			default:
				idLib::Printf( "Error: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n" );
				break;
		}
		byteOffset += sizeof( SYSTEM_LOGICAL_PROCESSOR_INFORMATION );
		ptr++;
	}

	free( buffer );

	return true;
}

/*
========================
Sys_GetCPUCacheSize
========================
*/
static void Sys_GetCPUCacheSize(const int level, int & count, int & size, int & lineSize ) {
	assert( level >= 1 && level <= 3 );
	cpuInfo_t cpuInfo;

	GetCPUInfo( cpuInfo );

	count = cpuInfo.cacheLevel[level - 1].count;
	size = cpuInfo.cacheLevel[level - 1].size;
	lineSize = cpuInfo.cacheLevel[level - 1].lineSize;
}

/*
========================
Sys_CPUCount

numLogicalCPUCores	- the number of logical CPU per core
numPhysicalCPUCores	- the total number of cores per package
numCPUPackages		- the total number of packages (physical processors)
========================
*/
static void Sys_CPUCount(size_t& numLogicalCPUCores, size_t& numPhysicalCPUCores, size_t& numCPUPackages ) {
	cpuInfo_t cpuInfo;
	GetCPUInfo( cpuInfo );

	numPhysicalCPUCores = cpuInfo.processorCoreCount;
	numLogicalCPUCores = cpuInfo.logicalProcessorCount;
	numCPUPackages = cpuInfo.processorPackageCount;
}

// ====== ARM / cross-platform: CPU 64-bit capability ======
//
// NOTE: Detecting "64-bit capable CPU" from a 32-bit ARM process is not
// standardized. We provide best-effort OS queries where available.

static inline bool Sys_ProcessIs64bit() noexcept {
	return sizeof(void*) == 8;
}

// Long mode (x64) capability via CPUID (not process bitness)
static inline bool Sys_X86is64bitCapable() {
	if (!HasCPUID())
	{
		return false;
	}
	uint32 regs[4] = {};
	CPUID(0x80000000u, 0u, regs);
	if (regs[_REG_EAX] < 0x80000001u)
	{
		return false;
	}
	CPUID(0x80000001u, 0u, regs);
	return (regs[_REG_EDX] & (1u << 29)) != 0; // Long Mode
}

static inline bool Sys_CPU64bitCapable() {
#if defined(ID_CPU_ARCH_X64)
	return true;                        // by definition
#elif defined(ID_CPU_ARCH_ARM64)
	return true;                        // by definition (AArch64 build)
#elif defined(ID_CPU_ARCH_X86)
	return Sys_X86is64bitCapable();   // CPUID long mode
#elif defined(ID_CPU_ARCH_ARM32)
#if defined(_WIN32)
	// Windows: ask the OS for native architecture if available
	typedef BOOL(WINAPI* IsWow64Process2Fn)(HANDLE, USHORT*, USHORT*);
	HMODULE h = GetModuleHandleW(L"kernel32.dll");
	if (h) {
		auto fn = reinterpret_cast<IsWow64Process2Fn>(GetProcAddress(h, "IsWow64Process2"));
		if (fn) {
			USHORT p = 0, n = 0;
			if (fn(GetCurrentProcess(), &p, &n)) {
				// PROCESSOR_ARCHITECTURE_ARM64 == 12, AMD64 == 9
				return (n == 12 /*ARM64*/ || n == 9 /*AMD64*/);
			}
		}
	}
	// Fallback: on old Windows, we can't reliably know from a 32-bit ARM proc.
	return false;
#elif defined(__APPLE__)
	// macOS/iOS: sysctl hw.optional.arm64 tells if the CPU/OS support arm64
	int val = 0; size_t sz = sizeof(val);
	if (0 == sysctlbyname("hw.optional.arm64", &val, &sz, nullptr, 0))
	{
		return val != 0;
	}
	// Older/intel Macs won't have this key in a 32-bit ARM build anyway.
	return false;
#elif defined(__linux__)
	// Best-effort: check /proc/cpuinfo for "AArch64" (heuristic)
	if (FILE* f = fopen("/proc/cpuinfo", "r")) {
		char buf[512] = {};
		while (fgets(buf, sizeof(buf), f)) {
			if (strstr(buf, "AArch64") || strstr(buf, "aarch64")) {
				fclose(f);
				return true;
			}
		}
		fclose(f);
	}
	// No reliable generic way from a 32-bit ARM userland; assume not.
	return false;
#else
	return false;
#endif
#else
	// Unknown arch: fall back to process bitness
	return Sys_ProcessIs64bit();
#endif
}

/*
================
Sys_GetCPUCapabilities
================
*/
cpuid_t Sys_GetCPUCapabilities() {
	int64 flags = 0;

	// Vendor
	if ( IsAMD() ) {
		flags = CPUID_AMD;
	} else if ( IsARM() ) {
		flags = CPUID_ARM;
	} else if ( IsIntel() ) {
		flags = CPUID_INTEL;
	} else {
		flags = CPUID_GENERIC;
	}

	// Bits
	if (Sys_CPU64bitCapable()) {
		flags |= CPUID_64BIT;
	} else {
		flags |= CPUID_32BIT;
	}

	// check for Hyper-Threading Technology
	if (HasSMT()) {
		flags |= CPUID_SMT;
	}

	// verify we're at least a Pentium or 486 with CPUID support
	if (((flags & CPUID_AMD) || (flags & CPUID_INTEL) || (flags & CPUID_GENERIC)) && HasCPUID())
	{
		// x86/x64 specific feature checks
		// check for Multi Media Extensions
		if (HasMMX()) {
			flags |= CPUID_MMX;
		}

		// check for 3DNow!
		if (Has3DNow()) {
			flags |= CPUID_3DNOW;
		}

		// check for Streaming SIMD Extensions
		if (HasSSE()) {
			flags |= CPUID_SSE | CPUID_FTZ;
		}

		// check for Streaming SIMD Extensions 2
		if (HasSSE2()) {
			flags |= CPUID_SSE2;
		}

		// check for Streaming SIMD Extensions 3 aka Prescott's New Instructions
		if (HasSSE3()) {
			flags |= CPUID_SSE3;
		}

		// check for Supplemental Streaming SIMD Extensions 3
		if (HasSSSE3()) {
			flags |= CPUID_SSSE3;
		}

		// check for Streaming SIMD Extensions 4.1
		if (HasSSE41()) {
			flags |= CPUID_SSE4_1;
		}

		// check for Streaming SIMD Extensions 4.2
		if (HasSSE42()) {
			flags |= CPUID_SSE4_2;
		}

		// check for Advanced Vector Extensions
		if (HasAVX()) {
			flags |= CPUID_AVX;
		}

		// check for Fused Multiply Add 3
		if (HasFMA3()) {
			flags |= CPUID_FMA3;
		}

		// check for Advanced Vector Extensions 2
		if (HasAVX2()) {
			flags |= CPUID_AVX2;
		}

		// check for Advanced Vector Extensions 512
		if (HasAVX512F()) {
			flags |= CPUID_AVX512F;
		}

		// check for Conditional Move (CMOV) and fast floating point comparison (FCOMI) instructions
		if (HasCMOV()) {
			flags |= CPUID_CMOV;
		}

		// check for Denormals-Are-Zero mode
		if (HasDAZ()) {
			flags |= CPUID_DAZ;
		}
	} else if (flags & CPUID_ARM) {
		// ARM-specific feature checks
		// check for NEON support
		if (HasNEON()) {
			flags |= CPUID_NEON;
		}

		// check for SVE support
		if (HasSVE()) {
			flags |= CPUID_SVE;
		}

		// check for SVE2 support
		if (HasSVE2()) {
			flags |= CPUID_SVE2;
		}

		// check for SVE2.1 support
		if (HasSVE21()) {
			flags |= CPUID_SVE2_1;
		}
	}

	return static_cast<cpuid_t>(flags);
}


/*
===============================================================================

	FPU

===============================================================================
*/

typedef struct bitFlag_s {
	const char *	name;
	int		    	bit;
} bitFlag_t;

static byte fpuState[128], *statePtr = fpuState;
static char fpuString[2048];
static bitFlag_t controlWordFlags[] = {
	{ "Invalid operation", 0 },
	{ "Denormalized operand", 1 },
	{ "Divide-by-zero", 2 },
	{ "Numeric overflow", 3 },
	{ "Numeric underflow", 4 },
	{ "Inexact result (precision)", 5 },
	{ "Infinity control", 12 },
	{ "", 0 }
};
static const char *precisionControlField[] = {
	"Single Precision (24-bits)",
	"Reserved",
	"Double Precision (53-bits)",
	"Double Extended Precision (64-bits)"
};
static const char *roundingControlField[] = {
	"Round to nearest",
	"Round down",
	"Round up",
	"Round toward zero"
};
static bitFlag_t statusWordFlags[] = {
	{ "Invalid operation", 0 },
	{ "Denormalized operand", 1 },
	{ "Divide-by-zero", 2 },
	{ "Numeric overflow", 3 },
	{ "Numeric underflow", 4 },
	{ "Inexact result (precision)", 5 },
	{ "Stack fault", 6 },
	{ "Error summary status", 7 },
	{ "FPU busy", 15 },
	{ "", 0 }
};

/*
===============
Sys_FPU_PrintStateFlags
===============
*/
static size_t Sys_FPU_PrintStateFlags( char *ptr, const int ctrl, const int stat, const int tags, const DWORD inof, const int inse, const DWORD opof, const int opse ) {
	size_t i = 0;
	size_t length = 0;

	length += sprintf( ptr+length,	"CTRL = %08x\n"
									"STAT = %08x\n"
									"TAGS = %08x\n"
									"INOF = %08lx\n"
									"INSE = %08x\n"
									"OPOF = %08lx\n"
									"OPSE = %08x\n"
									"\n",
									ctrl, stat, tags, inof, inse, opof, opse );

	length += sprintf( ptr+length, "Control Word:\n" );
	for ( i = 0; controlWordFlags[i].name[0]; i++ ) {
		length += sprintf( ptr+length, "  %-30s = %s\n", controlWordFlags[i].name, ( ctrl & ( 1 << controlWordFlags[i].bit ) ) ? "true" : "false" );
	}
	length += sprintf( ptr+length, "  %-30s = %s\n", "Precision control", precisionControlField[(ctrl>>8)&3] );
	length += sprintf( ptr+length, "  %-30s = %s\n", "Rounding control", roundingControlField[(ctrl>>10)&3] );

	length += sprintf( ptr+length, "Status Word:\n" );
	for ( i = 0; statusWordFlags[i].name[0]; i++ ) {
		ptr += sprintf( ptr+length, "  %-30s = %s\n", statusWordFlags[i].name, ( stat & ( 1 << statusWordFlags[i].bit ) ) ? "true" : "false" );
	}
	length += sprintf( ptr+length, "  %-30s = %d%d%d%d\n", "Condition code", (stat>>8)&1, (stat>>9)&1, (stat>>10)&1, (stat>>14)&1 );
	length += sprintf( ptr+length, "  %-30s = %d\n", "Top of stack pointer", (stat>>11)&7 );

	return numeric_cast<size_t>(length);
}

/*
===============
Sys_FPU_StackIsEmpty
===============
*/
bool Sys_FPU_StackIsEmpty() {
	__asm {
		mov			eax, statePtr
		fnstenv		[eax]
		mov			eax, [eax+8]
		xor			eax, 0xFFFFFFFF
		and			eax, 0x0000FFFF
		jz			empty
	}
	return false;
empty:
	return true;
}

/*
===============
Sys_FPU_ClearStack
===============
*/
void Sys_FPU_ClearStack() {
	__asm {
		mov			eax, statePtr
		fnstenv		[eax]
		mov			eax, [eax+8]
		xor			eax, 0xFFFFFFFF
		mov			edx, (3<<14)
	emptyStack:
		mov			ecx, eax
		and			ecx, edx
		jz			done
		fstp		st
		shr			edx, 2
		jmp			emptyStack
	done:
	}
}

/*
===============
Sys_FPU_GetState

  gets the FPU state without changing the state
===============
*/
const char *Sys_FPU_GetState() {
	double fpuStack[8] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	double *fpuStackPtr = fpuStack;
	int i = 0, numValues = 0;
	char *ptr;

	__asm {
		mov			esi, statePtr
		mov			edi, fpuStackPtr
		fnstenv		[esi]
		mov			esi, [esi+8]
		xor			esi, 0xFFFFFFFF
		mov			edx, (3<<14)
		xor			eax, eax
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fst			qword ptr [edi+0]
		inc			eax
		shr			edx, 2
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fxch		st(1)
		fst			qword ptr [edi+8]
		inc			eax
		fxch		st(1)
		shr			edx, 2
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fxch		st(2)
		fst			qword ptr [edi+16]
		inc			eax
		fxch		st(2)
		shr			edx, 2
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fxch		st(3)
		fst			qword ptr [edi+24]
		inc			eax
		fxch		st(3)
		shr			edx, 2
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fxch		st(4)
		fst			qword ptr [edi+32]
		inc			eax
		fxch		st(4)
		shr			edx, 2
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fxch		st(5)
		fst			qword ptr [edi+40]
		inc			eax
		fxch		st(5)
		shr			edx, 2
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fxch		st(6)
		fst			qword ptr [edi+48]
		inc			eax
		fxch		st(6)
		shr			edx, 2
		mov			ecx, esi
		and			ecx, edx
		jz			done
		fxch		st(7)
		fst			qword ptr [edi+56]
		inc			eax
		fxch		st(7)
	done:
		mov			numValues, eax
	}

	int ctrl = *reinterpret_cast<int*>(&fpuState[0]);
	int stat = *reinterpret_cast<int*>(&fpuState[4]);
	int tags = *reinterpret_cast<int*>(&fpuState[8]);
	int inof = *reinterpret_cast<int*>(&fpuState[12]);
	int inse = *reinterpret_cast<int*>(&fpuState[16]);
	int opof = *reinterpret_cast<int*>(&fpuState[20]);
	int opse = *reinterpret_cast<int*>(&fpuState[24]);

	ptr = fpuString;
	ptr += sprintf( ptr,"FPU State:\n"
						"num values on stack = %d\n", numValues );
	for ( i = 0; i < 8; i++ ) {
		ptr += sprintf( ptr, "ST%d = %1.10e\n", i, fpuStack[i] );
	}

	Sys_FPU_PrintStateFlags( ptr, ctrl, stat, tags, inof, inse, opof, opse );

	return fpuString;
}

/*
===============
Sys_FPU_EnableExceptions
===============
*/
void Sys_FPU_EnableExceptions( int exceptions ) {
	__asm {
		mov			eax, statePtr
		mov			ecx, exceptions
		and			cx, 63
		not			cx
		fnstcw		word ptr [eax]
		mov			bx, word ptr [eax]
		or			bx, 63
		and			bx, cx
		mov			word ptr [eax], bx
		fldcw		word ptr [eax]
	}
}

/*
===============
Sys_FPU_SetPrecision
===============
*/
void Sys_FPU_SetPrecision( int precision ) {
	short precisionBitTable[4] = { 0, 1, 3, 0 };
	short precisionBits = precisionBitTable[precision & 3] << 8;
	short precisionMask = ~( ( 1 << 9 ) | ( 1 << 8 ) );

	__asm {
		mov			eax, statePtr
		mov			cx, precisionBits
		fnstcw		word ptr [eax]
		mov			bx, word ptr [eax]
		and			bx, precisionMask
		or			bx, cx
		mov			word ptr [eax], bx
		fldcw		word ptr [eax]
	}
}

/*
================
Sys_FPU_SetRounding
================
*/
void Sys_FPU_SetRounding( uint8 rounding ) {
	short roundingBitTable[4] = { 0, 1, 2, 3 };
	short roundingBits = static_cast<short>(roundingBitTable[rounding & 3] << 10);
	short roundingMask = ~( ( 1 << 11 ) | ( 1 << 10 ) );

	__asm {
		mov			eax, statePtr
		mov			cx, roundingBits
		fnstcw		word ptr [eax]
		mov			bx, word ptr [eax]
		and			bx, roundingMask
		or			bx, cx
		mov			word ptr [eax], bx
		fldcw		word ptr [eax]
	}
}

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ( bool enable ) {
	DWORD dwData = 0;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 6
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<6)	// clear DAX bit
		or		eax, ecx		// set the DAZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ( bool enable ) {
	DWORD dwData = 0;

	_asm {
		movzx	ecx, byte ptr enable
		and		ecx, 1
		shl		ecx, 15
		STMXCSR	dword ptr dwData
		mov		eax, dwData
		and		eax, ~(1<<15)	// clear FTZ bit
		or		eax, ecx		// set the FTZ bit
		mov		dwData, eax
		LDMXCSR	dword ptr dwData
	}
}
