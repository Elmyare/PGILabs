#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

void display_bmp(const char *filename) {
    FILE *f;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char *pixels;
    int i, j, padding;

    // Открываем BMP файл
    f = fopen(filename, "rb");
    if (f == NULL) {
        perror("Ошибка открытия файла");
        return;
    }

    // Чтение заголовков
    fread(&fileHeader, sizeof(fileHeader), 1, f);
    fread(&infoHeader, sizeof(infoHeader), 1, f);

    // Проверка глубины цвета
    if (infoHeader.biBitCount != 4 && infoHeader.biBitCount != 8 && infoHeader.biBitCount != 24) {
        printf("Файл должен быть 16-цветным, 256-цветным или TrueColor BMP!\n");
        fclose(f);
        return;
    }

    // Чтение палитры (для 16 и 256 цветов)
    unsigned char palette[256][4];
    if (infoHeader.biBitCount == 4 || infoHeader.biBitCount == 8) {
        int colors = (infoHeader.biBitCount == 4) ? 16 : 256;
        fread(palette, 4, colors, f);
    }

    // Вычисление выравнивания
    int rowSize = (infoHeader.biWidth * infoHeader.biBitCount + 31) / 32 * 4;
    padding = rowSize - (infoHeader.biWidth * infoHeader.biBitCount / 8);

    // Выделение памяти для пикселей
    pixels = (unsigned char *)malloc(rowSize * infoHeader.biHeight);
    fread(pixels, 1, rowSize * infoHeader.biHeight, f);

    fclose(f);

    // Инициализация X11
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        printf("Ошибка подключения к X серверу!\n");
        free(pixels);
        return;
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0, infoHeader.biWidth, infoHeader.biHeight, 1,
        BlackPixel(display, screen), WhitePixel(display, screen)
    );

    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

    GC gc = XCreateGC(display, window, 0, NULL);

    // Ожидание отображения окна
    XEvent event;
    while (1) {
        XNextEvent(display, &event);
        if (event.type == Expose) {
            break;
        }
    }

    // Отображение пикселей
    for (i = 0; i < infoHeader.biHeight; i++) {
        for (j = 0; j < infoHeader.biWidth; j++) {
            unsigned char r, g, b;
            if (infoHeader.biBitCount == 4) {
                int index = (i * rowSize) + (j / 2);
                unsigned char pixel = pixels[index];
                if (j % 2 == 0) {
                    pixel = pixel >> 4;
                } else {
                    pixel = pixel & 0x0F;
                }
                r = palette[pixel][2];
                g = palette[pixel][1];
                b = palette[pixel][0];
            } else if (infoHeader.biBitCount == 8) {
                int index = (i * rowSize) + j;
                unsigned char pixel = pixels[index];
                r = palette[pixel][2];
                g = palette[pixel][1];
                b = palette[pixel][0];
            } else if (infoHeader.biBitCount == 24) {
                int index = (i * rowSize) + (j * 3);
                b = pixels[index];
                g = pixels[index + 1];
                r = pixels[index + 2];
            }
            XSetForeground(display, gc, (r << 16) | (g << 8) | b);
            XDrawPoint(display, window, gc, j, infoHeader.biHeight - i - 1);
        }
    }

    XFlush(display);

    // Ожидание закрытия окна
    while (1) {
        XNextEvent(display, &event);
        if (event.type == KeyPress) {
            break;
        }
    }

    // Освобождение ресурсов
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    free(pixels);
}

int main() {
    //display_bmp("CAT16.BMP");
    //display_bmp("CAT256.BMP");
    display_bmp("carib.bmp");
    return 0;
}