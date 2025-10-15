#include <Windows.h>
#include <iostream>
#include <fstream>

/*

void __noreturn KiUserExceptionDispatcher()
{
  __int64 v0; // r8
  unsigned int v1; // eax
  void *retaddr; // [rsp+0h] [rbp+0h] BYREF

  if ( Wow64PrepareForException )
	Wow64PrepareForException(&STACK[0x4F0], &retaddr);
  if ( (unsigned __int8)RtlDispatchException(&STACK[0x4F0], &retaddr) )
  {
	RtlGuardRestoreContext((PCONTEXT)&retaddr, 0i64);
  }
  else
  {
	LOBYTE(v0) = 0;
	v1 = ZwRaiseException(&STACK[0x4F0], &retaddr, v0);
  }
  RtlRaiseStatus(v1);
}
*/

// luh test app
typedef unsigned long long _QWORD;

int main();
void this_is_hooked();
int call_hooked_func();

void the_fucking_hook()
{
	printf("hi from %s.\r\n", __FUNCTION__);

	//return 3;
}



void this_is_hooked()
{
	printf("%s says hi\r\n", __FUNCTION__);
}

static HANDLE Handler = INVALID_HANDLE_VALUE;
static uintptr_t OriginalFunc = 0;
static uintptr_t HookedFunc = 0;
static DWORD OldProtection = 0;

// literally crash prevention & remove exception handler as first call
static LONG WINAPI CrashPrevention(EXCEPTION_POINTERS* pExceptionInfo)
{
	// could alwaays freeze each thread besides this here to make 'threadsafe' LOL

	printf("enter %s!\r\n", __FUNCTION__);
	printf("[%s] pExceptionInfo->ContextRecord->Rip = %p!\r\n", __FUNCTION__, pExceptionInfo->ContextRecord->Rip);

	if (Handler && Handler != INVALID_HANDLE_VALUE)
	{
		if (pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) //We will catch PAGE_GUARD Violation
		{
			// you NEED to unhook the func here, otherwise recursion.
			//printf("REMEMBER TO UNHOOK THE FUNCTION, THIS SERVES AS YOUR TRAMPOLINE..!\r\n");
			DWORD old;
			VirtualProtect((LPVOID)OriginalFunc, 1, OldProtection, &old); //Restore old Flags

			//pExceptionInfo->ContextRecord->Rip = pExceptionInfo->ContextRecord->Rip + 0x2;
			//pExceptionInfo->ContextRecord->Rip = (uintptr_t)OriginalFunc;

			printf("leave %s, this is where your code is called before the original is called!\r\n", __FUNCTION__);

			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	printf("WARNING: not our veh or cleaned, recursion?!!\r\n");

	// not our VEH or already cleaned up
	return EXCEPTION_CONTINUE_SEARCH;
}

//__int64(__fastcall* Wow64PrepareForExceptionViaTargetOriginal)(_QWORD, _QWORD);
//__int64 __fastcall Wow64PrepareForExceptionViaTargetHooked(_QWORD stack, _QWORD retaddr)
void(__fastcall* Wow64PrepareForExceptionViaTargetOriginal)(PEXCEPTION_RECORD, PCONTEXT);
void Wow64PrepareForExceptionViaTargetHooked(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT ContextRecord)
{
	printf("enter %s.\r\n", __FUNCTION__);

	static int CallCount = 0;

	printf("[%s] ExceptionRecord->ExceptionAddress = %p.\r\n", __FUNCTION__, ExceptionRecord->ExceptionAddress);
	//printf("[%s] ExceptionRecord->ExceptionRecord->ExceptionAddress = %p.\r\n", __FUNCTION__, ExceptionRecord->ExceptionRecord->ExceptionAddress);
	printf("[%s] ContextRecord->Rip = %p.\r\n", __FUNCTION__, ContextRecord->Rip);

	// the perfect thing about this, is that it is simply impossible for other ones to get ts since we WILL always be top :D
	if (CallCount == 0)
	{
		printf("first call.\r\n");

		Handler = AddVectoredExceptionHandler(true, (PVECTORED_EXCEPTION_HANDLER)CrashPrevention);
	}
	else
	{
		printf("second call.\r\n");

		CallCount = 0;
	}


	//printf("original rip = %p\r\n", ContextRecord->Rip);

	//ContextRecord->Rip = // (DWORD64)VEHHandler; // set rip to our actual handler. cus we need immediate execution... :3

	//Handler = AddVectoredExceptionHandler(true, (PVECTORED_EXCEPTION_HANDLER)VEHHandler);

	printf("leave %s.\r\n", __FUNCTION__);

	if (Wow64PrepareForExceptionViaTargetOriginal)
		return Wow64PrepareForExceptionViaTargetOriginal(ExceptionRecord, ContextRecord);

	return; // ret doesn't matter.
}

int call_hooked_func()
{
	printf("enter %s.\r\n", __FUNCTION__);

	this_is_hooked();

	printf("leave %s.\r\n", __FUNCTION__);

	return 2;
}

bool AreInSamePage(const uint8_t* Addr1, const uint8_t* Addr2)
{
	MEMORY_BASIC_INFORMATION mbi1;
	if (!VirtualQuery(Addr1, &mbi1, sizeof(mbi1)))
		return true;

	MEMORY_BASIC_INFORMATION mbi2;
	if (!VirtualQuery(Addr2, &mbi2, sizeof(mbi2)))
		return true;

	if (mbi1.BaseAddress == mbi2.BaseAddress)
		return true;

	return false;
}

bool Hook(uintptr_t original_func, uintptr_t hooked_func)
{
	uintptr_t ntdllBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"ntdll.dll"));
	uintptr_t KiUserExceptionDispatcherAddr = reinterpret_cast<uintptr_t>(GetProcAddress(reinterpret_cast<HMODULE>(ntdllBase), "KiUserExceptionDispatcher"));
	printf("KiUserExceptionDispatcher = %p!\r\n", KiUserExceptionDispatcherAddr);

	void* Wow64PrepareForException = nullptr;

	uint8_t* luhasm = reinterpret_cast<uint8_t*>(KiUserExceptionDispatcherAddr + 1);
	if (luhasm[0] == 0x48 && luhasm[1] == 0x8B && luhasm[2] == 0x05)
	{
		int32_t rel = *reinterpret_cast<int32_t*>(luhasm + 3);
		uintptr_t target = (uintptr_t)(luhasm + 7 + rel);
		void** wow64_ptr = reinterpret_cast<void**>(target);

		printf("Wow64PrepareForException value = %p\n", *wow64_ptr);

		DWORD prot;
		VirtualProtect(wow64_ptr, sizeof(void*), PAGE_READWRITE, &prot);
		Wow64PrepareForExceptionViaTargetOriginal = reinterpret_cast<decltype(Wow64PrepareForExceptionViaTargetOriginal)>(*wow64_ptr);

		*wow64_ptr = Wow64PrepareForExceptionViaTargetHooked;
		VirtualProtect(wow64_ptr, sizeof(void*), prot, &prot);
	}

	Wow64PrepareForExceptionViaTargetOriginal = reinterpret_cast<decltype(Wow64PrepareForExceptionViaTargetOriginal)>(Wow64PrepareForException);
	//*(void**)(Wow64PrepareForException) = Wow64PrepareForExceptionViaTargetHooked;

	printf("Wow64PrepareForException = 0x%p!\r\n", Wow64PrepareForException);

	OriginalFunc = original_func;
	HookedFunc = hooked_func;

	if (AreInSamePage((const uint8_t*)OriginalFunc, (const uint8_t*)HookedFunc))
	{
		printf("WARNING: SAME PAGE CAN CAUSE RECURSION, THIS SHOULD FAIL THE HOOK BUT WON'T TO TEST!\r\n");

		Sleep(5000);
	}

	if (VirtualProtect((LPVOID)OriginalFunc, 1, PAGE_EXECUTE_READ | PAGE_GUARD, &OldProtection))
	{
		printf("vp success!\r\n");

		return true;
	}

	printf("vp failed!\r\n");

	return false;
}



int main()
{
	bool hookres = Hook((uintptr_t)&this_is_hooked, (uintptr_t)&the_fucking_hook);
	printf("hook = %d!\r\n", hookres);

	// im lazy piece of shit

	printf("calling hook.\r\n");
	call_hooked_func();

	Sleep(2000);

	return 67;
}