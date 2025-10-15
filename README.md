### made while i was sleep deprived, and also at 3am.

### made mostly as a PoC, barely functioning. --- unfinished and won't be updated unless im bored (pull request it?)
- swaps the .data ptr for Wow64PrepareForException which is validated against nullptr and if its not nullptr, is then called (even in x64), and is the first code to run..?
- this has very low benefit and more risk (code-wise) as compared to typical pageguard, due to my shitcode.
- its mostly POC but u need to page guard the page again after the function returns so maybe page_guard the last instruction or something aswell, or maybe trap frame the first 2-3 instructions then once we get to the next ones, we just page_guard the first again? idfk im lazy asf.

# how it works?
```cpp
void __noreturn KiUserExceptionDispatcher()
{
  __int64 v0; // r8
  unsigned int v1; // eax
  void *retaddr; // [rsp+0h] [rbp+0h] BYREF

  if ( Wow64PrepareForException ) // <- swap this pointer out to our own
   // thus get called with same args as RtlDispatchException, which is PEXCEPTION_RECORD
    Wow64PrepareForException(&STACK[0x4F0], &retaddr); // <- swap this pointer out to our own
 
  // code execution before RtlDispatchException.!
  // thus install our own handler, so we can get called with RtlGuardRestoreContext, rather than crashing <3
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
```

```asm
; void __noreturn KiUserExceptionDispatcher()
.text:00000001800A0EE0 KiUserExceptionDispatcher proc near     ; DATA XREF: RtlpFunctionAddressTableEntry+23↑o
.text:00000001800A0EE0                                         ; .rdata:0000000180120B26↓o ...
.text:00000001800A0EE0                 cld
.text:00000001800A0EE1                 mov     rax, cs:Wow64PrepareForException  <---- this is the important part here
.text:00000001800A0EE8                 test    rax, rax
.text:00000001800A0EEB                 jz      short loc_1800A0EFC
.text:00000001800A0EED                 mov     rcx, rsp
.text:00000001800A0EF0                 add     rcx, 4F0h
.text:00000001800A0EF7                 mov     rdx, rsp
.text:00000001800A0EFA                 call    rax ; Wow64PrepareForException
.text:00000001800A0EFC
.text:00000001800A0EFC loc_1800A0EFC:                          ; CODE XREF: KiUserExceptionDispatcher+B↑j
.text:00000001800A0EFC                 mov     rcx, rsp
.text:00000001800A0EFF                 add     rcx, 4F0h
.text:00000001800A0F06                 mov     rdx, rsp
.text:00000001800A0F09                 call    RtlDispatchException
.text:00000001800A0F0E                 test    al, al
.text:00000001800A0F10                 jz      short loc_1800A0F1E
.text:00000001800A0F12                 mov     rcx, rsp        ; ContextRecord
.text:00000001800A0F15                 xor     edx, edx        ; ExceptionRecord
.text:00000001800A0F17                 call    RtlGuardRestoreContext
.text:00000001800A0F1C                 jmp     short loc_1800A0F33
.text:00000001800A0F1E ; ---------------------------------------------------------------------------
.text:00000001800A0F1E
.text:00000001800A0F1E loc_1800A0F1E:                          ; CODE XREF: KiUserExceptionDispatcher+30↑j
.text:00000001800A0F1E                 mov     rcx, rsp
.text:00000001800A0F21                 add     rcx, 4F0h
.text:00000001800A0F28                 mov     rdx, rsp
.text:00000001800A0F2B                 xor     r8b, r8b
.text:00000001800A0F2E                 call    ZwRaiseException
.text:00000001800A0F33
.text:00000001800A0F33 loc_1800A0F33:                          ; CODE XREF: KiUserExceptionDispatcher+3C↑j
.text:00000001800A0F33                 mov     ecx, eax
.text:00000001800A0F35                 call    RtlRaiseStatus
.text:00000001800A0F35 KiUserExceptionDispatcher endp
```

# thats about it. enjoy.
