#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static bool white_screen = false;

unsigned long next_color(Display *display, int screen) {
    if (white_screen) {
        white_screen = false;
        return WhitePixel(display, screen);
    }
    white_screen = true;
    return BlackPixel(display, screen);

}

int main(void) {
    Display* main_display = XOpenDisplay(0);
    Window root_window = XDefaultRootWindow(main_display);


    int default_screen = DefaultScreen(main_display);
    GC gc_ctx = XDefaultGC(main_display, default_screen);

    int window_x = 0;
    int window_y = 0;
    int window_width = 800;
    int window_height = 600;
    int border_width = 0;
    int window_depth = CopyFromParent;
    int window_class = CopyFromParent;
    Visual* window_visual = CopyFromParent;

    int attribute_value_mask = CWBackPixel | CWEventMask;
    XSetWindowAttributes window_attributes = {};
    window_attributes.background_pixel = 0xffafe9af;
    window_attributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;

    Window main_window = XCreateWindow(main_display, root_window,
            window_x, window_y, window_width, window_height,
            border_width, window_depth, window_class, window_visual,
            attribute_value_mask, &window_attributes);

    XMapWindow(main_display, main_window);
    XStoreName(main_display, main_window, "Handmade Hero on Linux");

    bool window_open = true;

    while (window_open) {
        XEvent general_event = {};
        XNextEvent(main_display, &general_event);
        switch (general_event.type) {
            case KeyPress:
            {
                XKeyPressedEvent *event = (XKeyPressedEvent *)&general_event;
                if (event->keycode == XKeysymToKeycode(main_display, XK_Escape)) {
                    window_open = false;
                } else if (event->keycode == XKeysymToKeycode(main_display, XK_q)) {
                    XWindowAttributes attrs;
                    XGetWindowAttributes(main_display, main_window, &attrs);
                    XSetForeground(main_display, gc_ctx, next_color(main_display, default_screen));
                    XFillRectangle(main_display, main_window, gc_ctx, 0, 0, attrs.width, attrs.height);
                }
            } break;

            case Expose:
            {
                XExposeEvent *event = (XExposeEvent *)&general_event;

                XSetForeground(main_display, gc_ctx, next_color(main_display, default_screen));
                XFillRectangle(main_display, main_window, gc_ctx, 0, 0, event->width, event->height);
            } break;

            default: {

            } break;
        }
    }

    return 0;
}