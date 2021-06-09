/********************************************************************
	File name:	c_scoped_hook.hpp
	Project  :	Cutie
	Author   :	Dor Cohen

    Reimplements Subhook's ScopedHookInstall and ScopedHookRemove using Subhook's
    C interface, because the provided classes don't support replacing stubs.

********************************************************************/
#ifndef CUTIE_C_SCOPED_HOOK_HPP
#define CUTIE_C_SCOPED_HOOK_HPP

#include <subhook.h>

namespace cutie {

#if defined SUBHOOK_X86_64
    static subhook_flags_t g_subhook_flags = SUBHOOK_64BIT_OFFSET;
#elif defined SUBHOOK_X86
    static subhook_flags_t g_subhook_flags = 0;
#else
#error Unsupported bitness
#endif

    class CScopedHookInstall {
    private:
        subhook_t* m_hook;
        void* m_src;

    public:
        CScopedHookInstall(subhook_t* hook, void* src, void* dst)
                : m_src(src), m_hook(hook) {
            *m_hook = subhook_new(m_src, dst, (subhook_flags_t) g_subhook_flags);
            subhook_install(*m_hook);
        }

        void Replace(void* dst) {
            subhook_remove(*m_hook);
            subhook_free(*m_hook);
            *m_hook = subhook_new(m_src, dst, (subhook_flags_t) g_subhook_flags);
            subhook_install(*m_hook);
        }

        ~CScopedHookInstall() {
            subhook_remove(*m_hook);
            subhook_free(*m_hook);
        }

    private:
        CScopedHookInstall(const CScopedHookInstall&) = delete;
        CScopedHookInstall& operator=(const CScopedHookInstall&) = delete;
    };

    class CScopedHookRemove {
    private:
        subhook_t* m_hook;

    public:
        CScopedHookRemove(subhook_t* hook)
                : m_hook(hook) {
            subhook_remove(*m_hook);
        }

        ~CScopedHookRemove() {
            subhook_install(*m_hook);
        }

    private:
        CScopedHookRemove(const CScopedHookRemove&);
        CScopedHookRemove& operator=(const CScopedHookRemove&);
    };

}

#endif //CUTIE_C_SCOPED_HOOK_HPP
