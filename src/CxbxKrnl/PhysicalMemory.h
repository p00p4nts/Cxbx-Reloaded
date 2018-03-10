// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// ******************************************************************
// *
// *    .,-:::::    .,::      .::::::::.    .,::      .:
// *  ,;;;'````'    `;;;,  .,;;  ;;;'';;'   `;;;,  .,;;
// *  [[[             '[[,,[['   [[[__[[\.    '[[,,[['
// *  $$$              Y$$$P     $$""""Y$$     Y$$$P
// *  `88bo,__,o,    oP"``"Yo,  _88o,,od8P   oP"``"Yo,
// *    "YUMMMMMP",m"       "Mm,""YUMMMP" ,m"       "Mm,
// *
// *   Cxbx->Win32->CxbxKrnl->PhysicalMemory.h
// *
// *  This file is part of the Cxbx project.
// *
// *  Cxbx and Cxbe are free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Bostom, MA 02111-1307, USA.
// *
// *  (c) 2017-2018      ergo720
// *
// *  All rights reserved
// *
// ******************************************************************

#ifndef PHYSICAL_MEMORY_H
#define PHYSICAL_MEMORY_H

#define _XBOXKRNL_DEFEXTRN_

namespace xboxkrnl
{
	#include <xboxkrnl/xboxkrnl.h>
};

#include "Emu.h"
#include "CxbxKrnl.h"
#include <windows.h>
#include <map>


/* Global typedefs */
typedef uintptr_t VAddr;
typedef uintptr_t PAddr;
typedef uint32_t u32;
typedef unsigned int PFN;
typedef unsigned int PFN_COUNT;


/* An entry of the list tracking the free pages on the system */
typedef struct _FreeBlock
{
	PFN start;                        // starting page of the block
	PFN_COUNT size;                   // number of pages in the block (edges included)
	xboxkrnl::LIST_ENTRY ListEntry;
}FreeBlock, *PFreeBlock;


// NOTE: all the bit fields below can have endianess issues...

/* The Xbox PTE, modelled around the Intel 386 PTE specification */
typedef struct _XBOX_PTE
{
	ULONG Valid : 1;
	ULONG Write : 1;
	ULONG Owner : 1;
	ULONG WriteThrough : 1;
	ULONG CacheDisable : 1;
	ULONG Accessed : 1;
	ULONG Dirty : 1;
	ULONG LargePage : 1;
	ULONG Global : 1;
	ULONG GuardOrEnd : 1; // software field used for our own purposes
	ULONG Persist : 1;    // software field used for our own purposes
	ULONG Unused : 1;     // software field used for our own purposes
	ULONG PFN : 20;
} XBOX_PTE, *PXBOX_PTE;


/* PTE as used by the memory manager */
typedef struct _MMPTE
{
	union
	{
		ULONG Default;
		XBOX_PTE Hardware;
	};
} MMPTE, *PMMPTE;


/* PFN entry used by the memory manager */
typedef struct _XBOX_PFN {
	union
	{
		ULONG Default;
		struct {
			ULONG LockCount : 16;  // Set to prevent page relocation. Used by MmLockUnlockPhysicalPage and others
			ULONG Busy : 1;        // If set, PFN is in use
			ULONG Unused : 1;
			ULONG PteIndex : 10;   // Offset in the PT that maps the pte (it seems to be needed only for page relocations)
			ULONG BusyType : 4;    // What the page is used for
		} Busy;
	};
} XBOX_PFN, *PXBOX_PFN;


/* enum describing the usage type of the memory pages */
enum PageType {
	Unknown,                   // Used by the PFN database
	Stack,                     // Used by MmCreateKernelStack
	VirtualPageTable,          // Used by the pages holding the PTs that map the user memory (lower 2 GiB)
	SystemPageTable,           // Used by the pages holding the PTs that map the system memory
	Pool,                      // Used by ExAllocatePoolWithTag
	VirtualMemory,             // Used by NtAllocateVirtualMemory
	SystemMemory,              // Used by MmAllocateSystemMemory
	Image,                     // Used by XeLoadSection
	Cache,                     // Used by the file cache related functions
	Contiguous,                // Used by MmAllocateContiguousMemoryEx and others
	Debugger,                  // xbdm-related
	COUNT                      // The size of the array containing the page usage per type
};


/* Lock count variables for the PFN database */
#define LOCK_COUNT_UNIT             2
#define LOCK_COUNT_MAXIMUM          0xFFFE


/* PTE protection masks */
#define PTE_VALID_MASK              0x00000001
#define PTE_WRITE_MASK              0x00000002
#define PTE_OWNER_MASK              0x00000004
#define PTE_WRITE_THROUGH_MASK      0x00000008
#define PTE_CACHE_DISABLE_MASK      0x00000010
#define PTE_ACCESS_MASK             0x00000020
#define PTE_DIRTY_MASK              0x00000040
#define PTE_LARGE_PAGE_MASK         0x00000080
#define PTE_GLOBAL_MASK             0x00000100
#define PTE_GUARD_END_MASK          0x00000200
#define PTE_PERSIST_MASK            0x00000400
#define PTE_NOACCESS                0x00000000
#define PTE_READONLY                0x00000000
#define PTE_READWRITE               PTE_WRITE_MASK
#define PTE_NOCACHE                 PTE_CACHE_DISABLE_MASK
#define PTE_GUARD                   PTE_GUARD_END_MASK
#define PTE_CACHE                   0x00000000
#define PTE_VALID_PROTECTION_MASK   0x0000021B
#define PTE_SYSTEM_PROTECTION_MASK  0x0000001B // valid, write, write-through, cache


/* Xbox PAGE Masks */
#define XBOX_PAGE_NOACCESS          0x01
#define XBOX_PAGE_READONLY          0x02
#define XBOX_PAGE_READWRITE         0x04
#define XBOX_PAGE_WRITECOPY         0x08 // ?
#define XBOX_PAGE_EXECUTE           0x10
#define XBOX_PAGE_EXECUTE_READ      0x20
#define XBOX_PAGE_EXECUTE_READWRITE 0x40
#define XBOX_PAGE_EXECUTE_WRITECOPY 0x80 // ?
#define XBOX_PAGE_GUARD             0x100
#define XBOX_PAGE_NOCACHE           0x200
#define XBOX_PAGE_WRITECOMBINE      0x400


/* Xbox MEM Masks */
#define XBOX_MEM_COMMIT             0x1000
#define XBOX_MEM_RESERVE            0x2000
#define XBOX_MEM_DECOMMIT           0x4000
#define XBOX_MEM_RELEASE            0x8000
#define XBOX_MEM_FREE               0x10000
#define XBOX_MEM_PRIVATE            0x20000
#define XBOX_MEM_MAPPED             0x40000 // ?
#define XBOX_MEM_RESET              0x80000
#define XBOX_MEM_TOP_DOWN           0x100000
#define XBOX_MEM_WRITE_WATCH        0x200000 // ?
#define XBOX_MEM_PHYSICAL           0x400000 // ?
#define XBOX_MEM_NOZERO             0x800000 // Replaces MEM_ROTATE on WinXP+
#define XBOX_MEM_IMAGE              0x1000000 // ?


/* Various macros to manipulate PDE/PTE/PFN */
#define GetPdeAddress(Va) ((PMMPTE)(((((ULONG)(Va)) >> 22) << 2) + PAGE_DIRECTORY_BASE)) // (Va/4M) * 4 + PDE_BASE
#define GetPteAddress(Va) ((PMMPTE)(((((ULONG)(Va)) >> 12) << 2) + PAGE_TABLES_BASE))    // (Va/4K) * 4 + PTE_BASE
#define GetVAddrMappedByPte(Pte) ((VAddr)((ULONG)(Pte) << 10))
#define GetPteOffset(Va) ((((ULONG)(Va)) << 10) >> 22)
#define WRITE_ZERO_PTE(pPte) ((pPte)->Default = 0)
#define WRITE_PTE(pPte, Pte) (*(pPte) = Pte)
#define PTE_PER_PAGE 1024
// On real hardware, enabling only the cache disable bit would result in an effective caching type of USWC
// (uncacheable speculative write combining), so we set both to achieve it
#define DISABLE_CACHING(Pte) ((Pte).Hardware.CacheDisable = 1); ((Pte).Hardware.WriteThrough = 1)
#define SET_WRITE_COMBINE(Pte) ((Pte).Hardware.CacheDisable = 0); ((Pte).Hardware.WriteThrough = 1)
#define ValidKernelPteBits (PTE_VALID_MASK | PTE_WRITE_MASK | PTE_DIRTY_MASK | PTE_ACCESS_MASK) // 0x63
#define ValidKernelPdeBits (PTE_VALID_MASK | PTE_WRITE_MASK | PTE_OWNER_MASK | PTE_DIRTY_MASK | PTE_ACCESS_MASK) // 0x67
// This returns the VAddr in the contiguous region
#define CONVERT_PFN_TO_CONTIGUOUS_PHYSICAL(Pfn) ((PCHAR)PHYSICAL_MAP_BASE + (Pfn << PAGE_SHIFT))
// This works with both PAddr and VAddr in the contiguous region
#define CONVERT_CONTIGUOUS_PHYSICAL_TO_PFN(Va) ((Va & (BYTES_IN_PHYSICAL_MAP - 1)) >> PAGE_SHIFT)
// This returns the address of the PFN entry for Xbox/Chihiro
#define XBOX_PFN_ELEMENT(pfn) (&((PXBOX_PFN)XBOX_PFN_ADDRESS)[pfn])
#define CHIHIRO_PFN_ELEMENT(pfn) (&((PXBOX_PFN)CHIHIRO_PFN_ADDRESS)[pfn])


/* Common page calculations */
#define ROUND_UP_4K(size) (((size) + PAGE_MASK) & (~PAGE_MASK))
#define ROUND_UP(size, alignment) (((size) + (alignment - 1)) & (~(alignment - 1)))
#define ROUND_DOWN_4K(size) ((size) & (~PAGE_MASK))
#define ROUND_DOWN(size, alignment) ((size) & (~(alignment - 1)))
#define CHECK_ALIGNMENT(size, alignment) (((size) % (alignment)) == 0)
#define PAGES_SPANNED(Va, Size) ((ULONG)((((VAddr)(Va) & (PAGE_SIZE - 1)) + (Size) + (PAGE_SIZE - 1)) >> PAGE_SHIFT))
#define BYTE_OFFSET(Va) ((ULONG)((VAddr)(Va) & (PAGE_SIZE - 1)))
#define BYTE_OFFSET_LARGE(Va) ((ULONG)((VAddr)(Va) & (PAGE_SIZE_LARGE - 1)))


/* These macros check if the supplied address is inside a known range */
#define IS_PHYSICAL_ADDRESS(Va) (((VAddr)(Va) - PHYSICAL_MAP_BASE) <= (PHYSICAL_MAP_END - PHYSICAL_MAP_BASE))
#define IS_SYSTEM_ADDRESS(Va) (((VAddr)(Va) - SYSTEM_MEMORY_BASE) <= (SYSTEM_MEMORY_END - SYSTEM_MEMORY_BASE))
#define IS_DEVKIT_ADDRESS(Va) (((VAddr)(Va) - DEVKIT_MEMORY_BASE) <= (DEVKIT_MEMORY_END - DEVKIT_MEMORY_BASE))
#define IS_USER_ADDRESS(Va) (((VAddr)(Va) - LOWEST_USER_ADDRESS) <= (HIGHEST_USER_ADDRESS - LOWEST_USER_ADDRESS))


/* Global helper function used to copy an ULONG block of memory to another buffer. It mimics RtlFillMemoryUlong */
void FillMemoryUlong(void* Destination, size_t Length, ULONG Long);

/* PhysicalMemory class */
class PhysicalMemory
{
	protected:
		// doubly linked list tracking the free physical pages
		xboxkrnl::LIST_ENTRY FreeList = { &FreeList , &FreeList };
		// highest pfn available for contiguous allocations
		PAddr m_MaxContiguousPfn = XBOX_CONTIGUOUS_MEMORY_LIMIT;
		// amount of free physical pages available for non-debugger usage
		PFN_COUNT m_PhysicalPagesAvailable = X64M_PHYSICAL_PAGE;
		// amount of free physical pages available for usage by the debugger
		PFN_COUNT m_DebuggerPagesAvailable = 0;
		// array containing the number of pages in use per type
		PFN_COUNT m_PagesByUsage[PageType::COUNT] = { 0 };
		// highest page on the system
		PFN m_HighestPage = XBOX_HIGHEST_PHYSICAL_PAGE;
		// first page of the nv2a instance memory
		PFN m_NV2AInstancePage = XBOX_INSTANCE_PHYSICAL_PAGE;
		// number of allocated bytes for the nv2a instance memory
		size_t m_NV2AInstanceMemoryBytes = NV2A_INSTANCE_PAGE_COUNT << PAGE_SHIFT;
		// boolean that indicates that the extra 64 MiB on a devkit can be used for heap/Nt allocations
		bool m_bAllowNonDebuggerOnTop64MiB = true;

	
		// protected constructor so PhysicalMemory can only be inherited from
		PhysicalMemory() {};
		// destructor
		~PhysicalMemory() {};
		// set up the page directory
		void InitializePageDirectory();
		// write a contiguous range of pfn's
		void WritePfn(PFN pfn_start, PFN pfn_end, PMMPTE pPte, PageType BusyType, bool bZero = false);
		// write a contiguous range of pte's
		void WritePte(PMMPTE pPteStart, PMMPTE pPteEnd, MMPTE Pte, PFN pfn, bool bZero = false);
		// commit a contiguous number of pages
		bool RemoveFree(PFN_COUNT NumberOfPages, PFN* result, PFN_COUNT PfnAlignment, PFN start, PFN end);
		// release a contiguous number of pages
		void InsertFree(PFN start, PFN end);
		// convert from Xbox to the desired system pte protection (if possible) and return it
		bool ConvertXboxToSystemPteProtection(DWORD perms, PMMPTE pPte);
		// convert from pte permissions to the corresponding Xbox protection code
		DWORD ConvertPteToXboxProtection(ULONG PteMask);
		// convert from Xbox to Windows permissions
		DWORD ConvertXboxToWinProtection(DWORD Perms);
		// add execute rights if the permission mask doesn't include it
		DWORD PatchXboxPermissions(DWORD Perms);
		// commit page tables (if necessary)
		bool AllocatePT(PFN_COUNT PteNumber, VAddr addr);
		// commit whatever free page is available and zero it
		PFN RemoveAndZeroAnyFreePage(PageType BusyType, PMMPTE pte, bool bPhysicalFunction);
		// checks if enough free pages are available for the allocation (doesn't account for fragmentation)
		bool IsMappable(PFN_COUNT PagesRequested, bool bRetailRegion, bool bDebugRegion);
};

#endif
