/********************************************************************
	File name:	mock_container.hpp
	Project  :	Cutie
	Author   :	t_dorco
	Created  :	14/06/2018

	Contains implementation needed for cmock_api.h
********************************************************************/
#ifndef CUTIE_MOCK_CONTAINER_HPP
#define CUTIE_MOCK_CONTAINER_HPP

#include <cmock/cmock.h>
#include <hook.hpp>

/********************************************************************
	A base class for CMock containers.
	Wraps SubHook's CScopedInstallHook class.
	Only used internally by MockContainer_##func
********************************************************************/
template<typename BaseClass>
class MockContainer : public ::testing::NiceMock<CMockMocker<BaseClass> > {
protected:
    MockContainer(void* func, void* stub) :
            m_hook(), m_install(&m_hook, func, stub) {}

    virtual ~MockContainer() {};

public:
    void set_stub(void* stub) {
        m_install.Replace(stub);
    }

private:
    MockContainer(const MockContainer&) = delete;
    MockContainer& operator=(const MockContainer&) = delete;

protected:
    subhook_t m_hook;
    cutie::CScopedHookInstall m_install;
};

#endif // CUTIE_MOCK_CONTAINER_HPP