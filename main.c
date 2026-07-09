#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable bool white_screen;
global_variable u_int8_t *bitmap_memory;
global_variable bool running = true;
global_variable int bytes_per_pixel = 4;

// the size of the bitmap array in bytes
global_variable size_t bitmap_size;

global_variable XImage *image;

unsigned long next_color(Display *display, int screen) {
    if (white_screen) {
        white_screen = false;
        return WhitePixel(display, screen);
    }
    white_screen = true;
    return BlackPixel(display, screen);

}

internal void render_weird_gradient(Display *display, Window window, GC graphics_ctx, int x_offset, int y_offset) {
    int pitch = image->width * bytes_per_pixel;
    uint8_t *row = bitmap_memory;

    for (int y = 0; y < image->height; ++y) {
        uint32_t *pixel = (uint32_t *)row;
        for (int x = 0; x < image->width; ++x) {
            uint8_t blue = x + x_offset;
            uint8_t green = y + y_offset;

            *pixel++ = green << 8 | blue;
        }

        row += pitch;
    }
    XPutImage(display, window, graphics_ctx, image, 0, 0, 0, 0, image->width, image->height);
    XFlush(display);
}

internal void resize_window(int width, int height) {
    size_t new_size = width * height * bytes_per_pixel;

    if (bitmap_memory == NULL) {
        bitmap_memory = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        bitmap_size = new_size;
    } else if (new_size > bitmap_size) {
        munmap(bitmap_memory, bitmap_size);
        // allocate an array twice the required size to avoid rapid allocations when resizing the window
        new_size *= 2;
        bitmap_memory = mmap(NULL, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        bitmap_size = new_size;
    }

    image->data = (char *)bitmap_memory;
    image->width = width;
    image->height = height;
    image->bytes_per_line = width * bytes_per_pixel;
}

int main(void) {
    Display* display = XOpenDisplay(0);
    Window root_window = XDefaultRootWindow(display);

    int screen = DefaultScreen(display);
    GC graphics_ctx = XDefaultGC(display, screen);

    int window_x = 0;
    int window_y = 0;
    int initial_width = 800;
    int initial_height = 600;
    int border_width = 0;
    int window_depth = CopyFromParent;
    int window_class = CopyFromParent;
    Visual* window_visual = CopyFromParent;

    int attribute_value_mask = CWBackPixmap | CWEventMask;
    XSetWindowAttributes window_attrs = {};
    window_attrs.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;

    Window window = XCreateWindow(display, root_window,
            window_x, window_y, initial_width, initial_height,
            border_width, window_depth, window_class, window_visual,
            attribute_value_mask, &window_attrs);

    XMapWindow(display, window);
    XStoreName(display, window, "Handmade Hero on Linux");

    image = XCreateImage(
        display,
        DefaultVisual(display, screen),
        DefaultDepth(display, screen),
        ZPixmap,
        0,
        NULL,
        initial_width,
        initial_height,
        32,
        0);

    // allocate the initial buffer for the window
    resize_window(initial_width, initial_height);
    int x_offset = 0;
    int y_offset = 0;

    while (running) {
        XEvent general_event = {};
        while (XPending(display) > 0) {
            XNextEvent(display, &general_event);
            switch (general_event.type) {
                case KeyPress: {
                    XKeyPressedEvent *event = (XKeyPressedEvent *)&general_event;
                    if (event->keycode == XKeysymToKeycode(display, XK_Escape)) {
                        running = false;
                    }
                } break;

                case Expose: {
                    XExposeEvent *event = (XExposeEvent *)&general_event;
                    resize_window(event->width, event->height);
                } break;

                default: {

                } break;
            }
        }
        render_weird_gradient(display, window, graphics_ctx, x_offset, y_offset);
        x_offset++;
    }

    return 0;
}