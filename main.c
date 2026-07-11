#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable bool running = true;

typedef struct {
    u_int8_t *memory;
    size_t size;
    XImage *image;
    Pixmap pixmap;
    int bytes_per_pixel;
    int window_width;
    int window_height;
    // pitch: handled by image->bytes_per_line
} x_offscreen_buffer;

#define BACK_BUFFER_WIDTH 1280
#define BACK_BUFFER_HEIGHT 720

internal void x_offscreen_buffer_init(x_offscreen_buffer *buf, Display* display, Drawable window, int screen) {
    buf->bytes_per_pixel = 4;
    buf->size = BACK_BUFFER_WIDTH * BACK_BUFFER_HEIGHT * buf->bytes_per_pixel;
    buf->window_width = BACK_BUFFER_WIDTH;
    buf->window_height = BACK_BUFFER_HEIGHT;
    buf->memory = mmap(NULL, buf->size, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    buf->pixmap = XCreatePixmap(display, window, BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT, DefaultDepth(display, screen));

    buf->image = XCreateImage(
       display,
       DefaultVisual(display, screen),
       DefaultDepth(display, screen),
       ZPixmap,
       0,
       (char *)buf->memory,
       BACK_BUFFER_WIDTH,
       BACK_BUFFER_HEIGHT,
       32,
       BACK_BUFFER_WIDTH * buf->bytes_per_pixel);
}

internal void render_back_buffer(x_offscreen_buffer buf, Display *display, Drawable window, GC graphics_ctx) {
    XRenderPictFormat *format = XRenderFindVisualFormat(display, DefaultVisual(display, 0));
    XPutImage(display, buf.pixmap, graphics_ctx, buf.image, 0, 0, 0, 0, buf.image->width, buf.image->height);
    Picture src_pict = XRenderCreatePicture(display, buf.pixmap, format, 0, NULL);
    Picture dst_pict =  XRenderCreatePicture(display, window, format, 0, NULL);

    double x_scale = BACK_BUFFER_WIDTH / (float)buf.window_width;
    double y_scale = BACK_BUFFER_HEIGHT / (float)buf.window_height;

    XTransform transform_matrix = {{
        {XDoubleToFixed(x_scale), 0, 0},
        {0, XDoubleToFixed(y_scale), 0},
        {0, 0, XDoubleToFixed(1.0)}
    }};
    XRenderSetPictureTransform(display, src_pict, &transform_matrix);

    XRenderSetPictureFilter(display, src_pict, "bilinear", NULL, 0);
    XRenderComposite(display, PictOpSrc, src_pict, 0, dst_pict, 0, 0, 0, 0, 0, 0, buf.window_width, buf.window_height);
    XRenderFreePicture(display, src_pict);
    XRenderFreePicture(display, dst_pict);
    XFlush(display);
}

internal void render_weird_gradient(x_offscreen_buffer buf, Display *display, Drawable window, GC graphics_ctx, int x_offset, int y_offset) {

    uint8_t *row = buf.memory;

    for (int y = 0; y < buf.image->height; ++y) {
        uint32_t *pixel = (uint32_t *)row;
        for (int x = 0; x < buf.image->width; ++x) {
            uint8_t blue = x + x_offset;
            uint8_t green = y + y_offset;

            *pixel++ = green << 8 | blue;
        }

        row += buf.image->bytes_per_line;
    }

    render_back_buffer(buf, display, window, graphics_ctx);
}

int main(void) {

    Display* display = XOpenDisplay(0);
    Window root_window = XDefaultRootWindow(display);
    
    int screen = DefaultScreen(display);
    GC graphics_ctx = XDefaultGC(display, screen);

    int attribute_value_mask = CWBackPixmap | CWEventMask;
    XSetWindowAttributes window_attrs = {};
    window_attrs.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;

    Window window = XCreateWindow(display, root_window,
            0, 0, BACK_BUFFER_WIDTH, BACK_BUFFER_HEIGHT,
            0, CopyFromParent, CopyFromParent, CopyFromParent,
            attribute_value_mask, &window_attrs);

    XMapWindow(display, window);
    XStoreName(display, window, "Handmade Hero on Linux");

    x_offscreen_buffer x_back_buffer;
    x_offscreen_buffer_init(&x_back_buffer, display, window, screen);

    int x_offset = 0;
    int y_offset = 0;

    while (running) {
        XEvent general_event;
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
                    x_back_buffer.window_width = event->width;
                    x_back_buffer.window_height = event->height;
                } break;

                default: {

                } break;
            }
        }
        render_weird_gradient(x_back_buffer, display, window, graphics_ctx, x_offset, y_offset);
        x_offset++;
        y_offset++;
    }

    return 0;
}