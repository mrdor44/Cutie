/********************************************************************
	File name:	mock.h
	Project  :	Cutie
	Author   :	Dor Cohen
	Created  :	30/06/2020

	Provides higher-level functionality for the CMock library.

	CMock
	~~~~~
	CMock is an extension for GMock, that enables setting mocks on C functions.
	CMock is needed as GMock doesn't support mocking C functions out-of-the-box.

	CMock vs. SubHook
	~~~~~~~~~~~~~~~~~
	SubHook allows stubbing C functions by setting hooks.
	The downside of using SubHook is that you need to define the stub yourself,
	even if it performs something as simple as returning a default value.

	CMock builds on top of SubHook and GMock to allow automatic definition
	of stubs in the form of mocks.

	CMock and SubHook can live side-by-side. You can even set a hook and a mock
	on the same function. The one that will be chosen is the one set last.

	How to mock?
	~~~~~~~~~~~~
	Assuming you use googletest as a UT	framework, lets write a test file:

		#include "Cutie/mock.hpp"

		extern "C" {
		#include <stdio.h>
		#include "mymodule.h"
		}

		using namespace testing;

		DECLARE_MOCKABLE(fopen, 2);
		DECLARE_MOCKABLE(fclose, 1);

		TEST(MYMODULE, fopen_fails) {
			INSTALL_EXPECT_CALL(fopen, _, _, _).WillRepeatedly(Return(-1));
			
			INSTALL_MOCK(fclose);
			CUTIE_ON_CALL(fclose, NotNull()).WillByDefault(Return(-1));
			CUTIE_EXPECT_CALL(fclose, _).WillOnce(Return(0));
			
			EXPECT_EQ(MYMODULE_calculate(), -1);
		}

	Lets review the relevant parts:

		* When #including mock.hpp, there's no need to #include gmock.h.
		* The `using` statement is for GMock's constructs.
		* DECLARE_MOCKABLE is used to declare fopen() and fclose() as mockable functions.
		  When declaring, you need to specify how many parameters each function has.
		* INSTALL_EXPECT_CALL() and CUTIE_EXPECT_CALL() are equivalent to GMock's EXPECT_CALL().
		  After using it you can use all constructs available with EXPECT_CALL.
		  In our example, we mock fopen() with any parameters received, and
		  we define the mock to always return -1.

    Quick Mocking vs. Full Mocking
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    To mock a function, we must first install the mock. After installing it, we can either set expectations or set
    default behavior (further explained later).

    Quick Mocking is possible using the INSTALL_EXPECT_CALL() and INSTALL_ON_CALL() macros. These macros install
    the mock and call EXPECT_CALL() or ON_CALL(), respectively.
    Quick Mocking a function is possible ONLY ONCE PER SCOPE, per function. For example, we can quick mock fopen()
    and fclose() in the same test, using INSTALL_EXPECT_CALL(), for example, however we can't use INSTALL_EXPECT_CALL()
    twice on fopen() in the same test.
    If we want to quick mock a function twice in the same test, we would have to switch to full mocking.

    Full mocking is the standard method for mocking. First, we install the mock using INSTALL_MOCK(), and then we can
    use CUTIE_EXPECT_CALL() and CUTIE_ON_CALL(), as many times as we want.

    Expectations vs. Default Behavior
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    We can set expectations or default behavior on a mock, or both.

    If a default behavior is set, calling the mocked function will result in the default behavior being triggered.
    Setting a default behavior doesn't verify that the mocked function was called.

    If an expectation is set, for example, .WillOnce(Return(1)), calling the mocked function will result in the
    given behavior, just like default behavior. However, it will also verify that the mocked function was called.
    Essentially, the difference between expectations and default behavior is that expectations verify that the
    function was called.

    Summary Table
    ~~~~~~~~~~~~~
    This table summarizes your mocking options:

                              | Quick Mocking       | Full Mocking      |
        ----------------------+---------------------+-------------------+
        Set Expectations      | INSTALL_EXPECT_CALL | CUTIE_EXPECT_CALL |
        ----------------------+---------------------+-------------------+
        Set Default Behavior  | INSTALL_ON_CALL     | CUTIE_ON_CALL     |
        ----------------------+---------------------+-------------------+

	Example: Setting expectations in scopes
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	To set expectations in scopes, use the following:

		DECLARE_MOCKABLE(fclose, 1);

		TEST(MYMODULE, setting_expectations_in_scope) {
			INSTALL_MOCK(fclose);

			InSequence s;
			for (int i = 0; i < 4; ++i) {
				CUTIE_EXPECT_CALL(fclose, 0).WillOnce(Return(i));
			}
		}

	Lets review the relevant parts:

		* INSTALL_MOCK() must be called if you intend to use CUTIE_EXPECT_CALL.
		  If you're using INSTALL_EXPECT_CALL, don't invoke INSTALL_MOCK.
		* You can't mix calls to CUTIE_EXPECT_CALL and INSTALL_EXPECT_CALL to the same
		  function in the same TEST.
		  You can of course mix them for different functions.
		* CUTIE_EXPECT_CALL() is used for setting expectations in a scope.
		* InSequence enforces the expectations that follow to be sequential.
		  If not using InSequence, you must add .RetiresOnSaturation() to the
		  expectation inside the loop, like this:
		  	CUTIE_EXPECT_CALL(...).WillOnce(...).RetiresOnSaturation();

	Breaking up INSTALL_HOOK
 	~~~~~~~~~~~~~~~~~~~~~~~~
 	The INSTALL_HOOK macro creates and initialized a Mock Container (an internal implementation class that allows
 	mocking). The container is basically a variable. It's pretty much like defining:
 	
 	    MyExmapleMockContainer __cmock__my_example_container(... parameters ...);
 	   
 	Most of the time, you will use INSTALL_EXPECT_CALL, which already declares and initialized the container, or
 	CUTIE_EXPECT_CALL in conjunction with INSTALL_HOOK that delares and initialized the container.

 	Sometimes you will want to split the container's declaration and initialization to different places.
 	For that, CUTIE_UNINITIALIZED_CONTAINER and CUTIE_INITIALIZE_CONTAINER should be used.
 	Example:
 	
 	    CUTIE_UNINITIALIZED_CONTAINER(my_func);
 	    CUTIE_INITIALIZE_CONTAINER(my_func);
 	    
 	This is pretty useful if you want to declare the Mock Container as a class field, and initialize it in a function
 	or in the constructor.
 
 	Ellipsis
	~~~~~~~~
	CMock doesn't support mocking functions with ellipsis.
	It may be possible if the number of arguments is known, but is not tested.

********************************************************************/
#ifndef CUTIE_MOCK_HPP
#define CUTIE_MOCK_HPP

#include "inc/mock_container.hpp"

/********************************************************************
	@brief Declare a function as mockable. Must be called once for
		every function that will be mocked, or else mocking won't work.

	@param func [IN] The function name to mark as mockable
	@param num_params [IN] The number of parameters
********************************************************************/
#define DECLARE_MOCKABLE(func, num_params) \
    class MockContainer_##func : public MockContainer<MockContainer_##func> { \
    public: \
        MockContainer_##func() : MockContainer<MockContainer_##func>((void*)(func), nullptr) {} \
        explicit MockContainer_##func(void* stub) : MockContainer<MockContainer_##func>((void*)(func), stub) {} \
        MOCK_METHOD##num_params(__CMOCK_STUB__##func, decltype(func)); \
    }; \
    CMOCK_MOCK_FUNCTION##num_params(MockContainer_##func, __CMOCK_STUB__##func, decltype(func)); \
    static_assert(true, "Semicolon required")

/********************************************************************
	@brief Declare an uninitialized Mock Container.
 	  This is useful when declaring the container as a field of a class.
 	  After declaring it, you MUST initialize it using CUTIE_INITIALIZE_CONTAINER

	@param func [IN] The function name to mock
********************************************************************/
#define CUTIE_UNINITIALIZED_CONTAINER(func) MockContainer_##func __cmock__##func

/********************************************************************
	@brief Initialize an uninitialized container declared with CUTIE_UNINITIALIZED_CONTAINER

	@param func [IN] The function name to mock
********************************************************************/
#define CUTIE_INITIALIZE_CONTAINER(func) __cmock__##func.set_stub((void*)__CMOCK_STUB__##func)

/********************************************************************
	@brief Declare and initialize a mock, without setting expectations or default behavior on it.
		Required for using CUTIE_EXPECT_CALL.
		When using INSTALL_EXPECT_CALL, this is not needed.

	@param func [IN] The function name to mock
********************************************************************/
#define INSTALL_MOCK(func) CUTIE_UNINITIALIZED_CONTAINER(func)((void*)__CMOCK_STUB__##func)

/********************************************************************
	@brief The equivalent of GMock's EXPECT_CALL.
 		   Use in conjunction with INSTALL_MOCK.
 		   
 		   The difference between GMock's EXPECT_CALL and CUTIE_EXPECT_CALL is:

				EXPECT_CALL(myfunc(1, 2))
				CUTIE_EXPECT_CALL(myfunc, 1, 2)

	@param func [IN] The function name to mock
	@param ... [IN] The expected parameters of the function.
********************************************************************/
#define CUTIE_EXPECT_CALL(func, ...) EXPECT_CALL(__cmock__##func, __CMOCK_STUB__##func(__VA_ARGS__))

/********************************************************************
	@brief The equivalent of GMock's EXPECT_CALL, to use without INSTALL_MOCK.

	@param func [IN] The function name to mock
	@param ... [IN] The expected parameters of the function.
********************************************************************/
#define INSTALL_EXPECT_CALL(func, ...) INSTALL_MOCK(func); CUTIE_EXPECT_CALL(func, __VA_ARGS__)

/********************************************************************
	@brief The equivalent of GMock's ON_CALL.
 		   Use in conjunction with INSTALL_MOCK.
 		   
 		   The difference between GMock's ON_CALL and ON_CALL is:

				ON_CALL(myfunc(1, 2))
				CUTIE_ON_CALL(myfunc, 1, 2)

	@param func [IN] The function name to mock
	@param ... [IN] The expected parameters of the function.
********************************************************************/
#define CUTIE_ON_CALL(func, ...) ON_CALL(__cmock__##func, __CMOCK_STUB__##func(__VA_ARGS__))

/********************************************************************
	@brief The equivalent of GMock's ON_CALL, to use without INSTALL_MOCK.

	@param func [IN] The function name to mock
	@param ... [IN] The expected parameters of the function.
********************************************************************/
#define INSTALL_ON_CALL(func, ...) INSTALL_MOCK(func); CUTIE_ON_CALL(func, __VA_ARGS__)

#endif // CUTIE_MOCK_HPP