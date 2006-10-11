#ifndef NIV_WINDOW_HPP__
#define NIV_WINDOW_HPP__

#include "xlib.hpp"

namespace NIV
{
    class Window
    {
        static const int x = 0;
        static const int y = 0;

        private:
            Window(const Window&);
            Window& operator=(const Window&);
        public:
            Window(int width, int height);
            ~Window()
            {
                printf("Closing Window\n");
            }

            void startEventLoop();

        private:
            DisplayWrapper displayWrapper;
            XVisualInfoWrapper visinfoWrapper;
            GLXContextWrapper ctxWrapper;

        public:
            Display * const dpy;

        private:
            int getScreenNum();
            void setupWindow(int width, int height);

            void resize(int width, int height);
            void initGL();

            Atom wm_delete_window;
        public:
            ::Window win;
    };
}

#endif

