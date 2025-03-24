#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#pragma pack(push, 1)
typedef struct {
    unsigned char ID;
    unsigned char Version;
    unsigned char Coding;
    unsigned char BitPerPixel;
    unsigned short XMin;
    unsigned short YMin;
    unsigned short XMax;
    unsigned short YMax;
    unsigned short HRes;
    unsigned short VRes;
    unsigned char Palette[48];
    unsigned char Reserved;
    unsigned char Planes;
    unsigned short BytePerLine;
    unsigned short PaletteInfo;
    unsigned short HScreenSize;
    unsigned short VScreenSize;
    unsigned char Filler[54];
} TPCXHeader;
#pragma pack(pop)

void display_pcx(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Ошибка открытия PCX файла");
        return;
    }

    TPCXHeader header;
    fread(&header, sizeof(header), 1, f);

    int width = header.XMax - header.XMin + 1;
    int height = header.YMax - header.YMin + 1;
    printf("Размер изображения: %d x %d\n", width, height);

    // Проверяем поддержку формата
    if (header.ID != 0x0A || header.Coding != 1 || header.BitPerPixel != 8 || header.Planes != 1) {
        printf("Поддерживаются только 256-цветные PCX!\n");
        fclose(f);
        return;
    }

    // Читаем изображение (RLE-декодирование)
    unsigned char *imageData = (unsigned char *)malloc(width * height);
    if (!imageData) {
        printf("Ошибка выделения памяти!\n");
        fclose(f);
        return;
    }

    int row, col, index = 0;
    unsigned char byte, count;
    for (row = 0; row < height; row++) {
        col = 0;
        while (col < header.BytePerLine) {
            fread(&byte, 1, 1, f);
            if ((byte & 0xC0) == 0xC0) {  // Если это RLE
                count = byte & 0x3F;
                fread(&byte, 1, 1, f);
            } else {
                count = 1;
            }

            while (count-- && col < header.BytePerLine) {
                if (col < width) {
                    imageData[row * width + col] = byte;
                }
                col++;
            }
        }
    }

    // Читаем 256-цветную палитру (последние 769 байтов: 1 байт 0x0C + 256 * 3)
    fseek(f, -769, SEEK_END);
    unsigned char palette[256][3];
    fread(&byte, 1, 1, f);  // Должно быть 0x0C
    if (byte != 0x0C) {
        printf("Ошибка в палитре PCX!\n");
        free(imageData);
        fclose(f);
        return;
    }
    fread(palette, 3, 256, f);
    fclose(f);

    // --- X11 отрисовка ---
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        printf("Ошибка подключения к X серверу!\n");
        free(imageData);
        return;
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                        0, 0, width, height, 1,
                                        BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

    GC gc = XCreateGC(display, window, 0, NULL);

    // Ждем события Expose перед отрисовкой
    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        if (event.type == Expose) {
            break;
        }
    }

    // Отрисовка пикселей (исправлено: теперь правильно идет индексирование!)
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            unsigned char pixel = imageData[row * width + col];
            unsigned char r = palette[pixel][0];
            unsigned char g = palette[pixel][1];
            unsigned char b = palette[pixel][2];

            XSetForeground(display, gc, (r << 16) | (g << 8) | b);
            XDrawPoint(display, window, gc, col, row);
        }
    }

    XFlush(display);

    // Ждем закрытия окна
    while (1) {
        XNextEvent(display, &event);
        if (event.type == KeyPress) {
            break;
        }
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    free(imageData);
}

int main() {
    display_pcx("200001.PCX");
    return 0;
}
