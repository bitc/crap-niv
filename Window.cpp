#include <GL/gl.h>

#include "Window.hpp"

#include "xlib.hpp"


int NIV::Window::getScreenNum()
{
    Screen* screen = XDefaultScreenOfDisplay(displayWrapper.ptr);
    return XScreenNumberOfScreen(screen);
}

NIV::Window::Window(int width, int height) :
    displayWrapper(NULL),
    visinfoWrapper(displayWrapper.ptr, getScreenNum()),
    ctxWrapper(displayWrapper.ptr, visinfoWrapper.ptr, NULL, True),
    dpy(displayWrapper.ptr)
{
    setupWindow(width, height);
    resize(width, height);
    initGL();
}

void NIV::Window::setupWindow(int width, int height)
{
    ::Window root = XRootWindow(dpy, getScreenNum());

    XSetWindowAttributes attr;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(dpy, root, visinfoWrapper.ptr->visual, AllocNone);
    attr.event_mask =
        StructureNotifyMask |
        ExposureMask |
        KeyPressMask;

    unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

    win = XCreateWindow(dpy, root, x, y, width, height,
            0, visinfoWrapper.ptr->depth, InputOutput,
            visinfoWrapper.ptr->visual, mask, &attr );

    visinfoWrapper.destroy();

    /* This variable will store the newly created property. */
    XTextProperty window_title_property;

    /* This is the string to be translated into a property. */
    char* window_title = "hello, world";

    /* translate the given string into an X property. */
    int rc = XStringListToTextProperty(&window_title,
            1,
            &window_title_property);

    /* check the success of the translation. */
    if (rc)
    {
        /* assume that window_title_property is our XTextProperty, and is */
        /* defined to contain the desired window title.                   */
        XSetWMName(dpy, win, &window_title_property);
    }


    Atom wm_protocols     = XInternAtom(dpy, "WM_PROTOCOLS", 1);
    wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", 1);
    XSetWMProtocols (dpy, win, &wm_protocols, 1);
    XSetWMProtocols (dpy, win, &wm_delete_window, 1);

    XMapWindow(dpy, win);
    glXMakeCurrent(dpy, win, ctxWrapper.handle);
}

void NIV::Window::startEventLoop()
{
    bool done = false;
    while(!done)
    {
        while (XPending(dpy) > 0)
        {
            XEvent event;
            XNextEvent(dpy, &event);
            printf("Event %d\n", event.type);
            switch (event.type)
            {
                case Expose:
                    printf("Expose\n");
                    break;
                case ConfigureNotify:
                    resize(event.xconfigure.width, event.xconfigure.height);
                    break;
                case ClientMessage:
                    if (static_cast<Atom>(event.xclient.data.l[0]) == wm_delete_window)
                    {
                        printf("WM_DELETE_WINDOW\n");
                        done = true;
                    }
            }

            glClear(GL_COLOR_BUFFER_BIT);
            glXSwapBuffers(dpy, win);
        }
    }
}

void NIV::Window::resize(int width, int height)
{
    glViewport(0, 0, (GLint)width, (GLint)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void NIV::Window::initGL()
{
    glClearColor(0, 0, 1, 1);
}

