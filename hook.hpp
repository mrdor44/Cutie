/********************************************************************
	File name:	hook.h
	Project  :	Cutie
	Author   :	Dor Cohen
	Created  :	30/6/2020

	Provides higher-level hooks functionality based on the Subhook library.

	Subhook
	-------
	SubHook is a super-simple hooking library for C/C++ that works on Linux and
	Windows. It currently supports x86 and x86-64.

	When should I use hooks?
	------------------------
	Hooking is widely used while writing UT for a module.
	Suppose your module exposes the following function:

		int MYMODULE_calculate();

	The function calls several API calls, among is the function:

		FILE* fopen(const char* path, const char* mode);

	Assume we want to write a test in which fopen() fails, and we want to verify
	that MYMODULE_calculate() can handle it (and probably return an error code).

	How to hook?
	-----------
	Lets write a test file that tests the scenario depicted above:

		#include "Cutie/mock.hpp"

		extern "C" {
		#include <stdio.h>
		#include "mymodule.h"
		}

		DECLARE_HOOKABLE(fopen);

		int __STUB__fopen_fail(const char* path, const char* mode) {
			SCOPE_REMOVE_HOOK(fopen);
			return -1;
		}

		TEST(MYMODULE, fopen_fails) {
			INSTALL_HOOK(fopen, __STUB__fopen_fails);
			EXPECT_EQ(MYMODULE_calculate(), -1);
		}

	Lets review the relevant parts:

		* When #including C header files, make sure to #include them as an extern "C".
		* DECLARE_HOOKABLE is used to declare fopen() as hookable.
		* __STUB__fopen_fail is the stub function that will be called instead of fopen().
		* SCOPE_REMOVE_HOOK is required if you wish to invoke the original fopen()
		  from within your stub function. Omitting it will cause any calls to fopen()
		  from within the stub to call the hook function, resulting in an infinite loop.
		  If you don't call the original function from within the stub, the line can be omitted.
		* INSTALL_HOOK() installs a hook on fopen() that will invoke the stub.

********************************************************************/
#ifndef CUTIE_HOOK_HPP
#define CUTIE_HOOK_HPP

#include "inc/c_scoped_hook.hpp"

/********************************************************************
	@brief Declare a function as hookable. Must be called once for
		every function that will be hooked, or else INSTALL_HOOK
		won't work.

	@param func [IN] The function name to mark as hookable
********************************************************************/
#define DECLARE_HOOKABLE(func) subhook_t __hook__##func

/********************************************************************
	@brief Install a hook on a function. The hook takes place
		immediately. The hook is removed when scope ends.

	@param func [IN] The function to place a hook on
	@param stub [IN] The function that will be called
********************************************************************/
#define INSTALL_HOOK(func, stub) \
    cutie::CScopedHookInstall __install__##func(&(__hook__##func), (void*)(func), (void*)(stub))

/********************************************************************
	@brief Replace a currently installed hook.

	@param func [IN] The function to place a hook on
	@param stub [IN] The function that will be called
********************************************************************/
#define REPLACE_HOOK(func, stub) (__install__##func.Replace((void*)(stub)))

/********************************************************************
	@brief This should be called from within the stub only.
		Call this if you want to call the original function from
		within the stub. Failure to call this will cause the hook
		to be called infinitely.

	@param func [IN] The function that was hooked
********************************************************************/
#define SCOPE_REMOVE_HOOK(func) cutie::CScopedHookRemove __remove__##__LINE__(&(__hook__##func))

#endif // CUTIE_HOOK_HPP
