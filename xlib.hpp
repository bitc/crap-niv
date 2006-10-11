#ifndef NIV_XLIB_HPP__
#define NIV_XLIB_HPP__

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <GL/glx.h>

#include <stdio.h>

namespace NIV
{

    class DisplayWrapper
    {
        private:
            DisplayWrapper(const DisplayWrapper&);
            DisplayWrapper& operator=(const DisplayWrapper&);
        public:
            Display* ptr;
            DisplayWrapper(const char* display_name)
            {
                printf("Creating Display...\n");
                ptr = XOpenDisplay(display_name);
                if (!ptr)
                    throw 1;
                printf("Done.\n");
            }

            ~DisplayWrapper()
            {
                printf("Closing Display...\n");
                XCloseDisplay(ptr);
                printf("Done.\n");
            }
    };

    class XVisualInfoWrapper
    {
        private:
            XVisualInfoWrapper(const XVisualInfoWrapper&);
            XVisualInfoWrapper& operator=(const XVisualInfoWrapper&);
        public:
            XVisualInfo *ptr;
            XVisualInfoWrapper(Display *dpy, int screen)
            {
                printf("Creating VisualInfo...\n");

                int attrib[] = {
                    GLX_RGBA,
                    GLX_RED_SIZE, 1,
                    GLX_GREEN_SIZE, 1,
                    GLX_BLUE_SIZE, 1,
                    GLX_DOUBLEBUFFER,
                    None };

                ptr = glXChooseVisual(dpy, screen, attrib);
                if (!ptr)
                    throw 1;
                printf("Done.\n");
            }
            ~XVisualInfoWrapper()
            {
                destroy();
            }
            void destroy()
            {
                if (ptr)
                {
                    ptr = NULL;
                    printf("Freeing VisualInfo...\n");
                    XFree(ptr);
                    printf("Done.\n");
                }
            }
    };

    class GLXContextWrapper
    {
        private:
            GLXContextWrapper(const GLXContextWrapper& rhs);
            GLXContextWrapper& operator=(GLXContextWrapper& rhs);
        public:
            GLXContext handle;
            GLXContextWrapper(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct)
                : display(dpy)
            {
                printf("Creating GLXContext...\n");
                handle = glXCreateContext(dpy, vis, shareList, direct);
                if (!handle)
                    throw 1;
                printf("Done.\n");
            }
            ~GLXContextWrapper()
            {
                printf("Destroying GLXContext...\n");
                glXDestroyContext(display, handle);
                printf("Done.\n");
            }
        private:
            Display *display;
    };
}

#endif

