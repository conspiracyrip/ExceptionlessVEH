### made while i was sleep deprived, and also at 3am.

### made mostly as a PoC, barely functioning. --- unfinished and won't be updated (pull request it?)
swaps the .data ptr for Wow64PrepareForException which is validated against nullptr and if its not nullptr, is then called (even in x64), and is the first code to run..?
this has very low benefit and more risk (code-wise) as compared to typical pageguard, due to my shitcoded.
its mostly POC but u need to page guard the page again after the function returns so maybe page_guard the last instruction or something aswell, or maybe trap frame the first 2-3 instructions then once we get to the next ones, we just page_guard the first again? idfk im lazy asf.

# how it works?
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

# uh if you find use for this yeah idk enjoy i guess
