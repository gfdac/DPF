/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "ApplicationPrivateData.hpp"
#include "../Window.hpp"

#include "pugl-upstream/include/pugl/pugl.h"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

Application::PrivateData::PrivateData(const bool standalone)
    : world(puglNewWorld(standalone ? PUGL_PROGRAM : PUGL_MODULE,
                         standalone ? PUGL_WORLD_THREADS : 0x0)),
      isStandalone(standalone),
      isStarting(true),
      isQuitting(false),
      visibleWindows(0),
      windows(),
      idleCallbacks()
{
    DISTRHO_SAFE_ASSERT_RETURN(world != nullptr,);

    puglSetWorldHandle(world, this);
    puglSetClassName(world, DISTRHO_MACRO_AS_STRING(DGL_NAMESPACE));

    // puglSetLogLevel(world, PUGL_LOG_LEVEL_DEBUG);

}

Application::PrivateData::~PrivateData()
{
    DISTRHO_SAFE_ASSERT(!isStarting);
    DISTRHO_SAFE_ASSERT(isQuitting);
    DISTRHO_SAFE_ASSERT(visibleWindows == 0);

    windows.clear();
    idleCallbacks.clear();

    if (world != nullptr)
        puglFreeWorld(world);
}

// --------------------------------------------------------------------------------------------------------------------

void Application::PrivateData::oneWindowShown() noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(isStandalone,);

    if (++visibleWindows == 1)
    {
        isStarting = false;
        isQuitting = false;
    }
}

void Application::PrivateData::oneWindowHidden() noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(isStandalone,);
    DISTRHO_SAFE_ASSERT_RETURN(visibleWindows != 0,);

    if (--visibleWindows == 0)
        isQuitting = true;
}

void Application::PrivateData::idle(const uint timeoutInMs)
{
    puglUpdate(world, timeoutInMs == 0 ? 0.0 : static_cast<double>(timeoutInMs) / 1000.0);

    for (std::list<Window*>::iterator it = windows.begin(), ite = windows.end(); it != ite; ++it)
    {
        Window* const window(*it);
        window->_idle();
    }

    for (std::list<IdleCallback*>::iterator it = idleCallbacks.begin(), ite = idleCallbacks.end(); it != ite; ++it)
    {
        IdleCallback* const idleCallback(*it);
        idleCallback->idleCallback();
    }
}

void Application::PrivateData::quit()
{
    isQuitting = true;

    for (std::list<Window*>::reverse_iterator rit = windows.rbegin(), rite = windows.rend(); rit != rite; ++rit)
    {
        Window* const window(*rit);
        window->close();
    }
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
