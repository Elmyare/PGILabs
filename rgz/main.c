#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

typedef struct {
    unsigned char r, g, b;
} RGB;

typedef struct {
    RGB *colors;
    int count;
} ColorBox;

// Функция для вычисления расстояния между двумя цветами
double color_distance(RGB c1, RGB c2) {
    return sqrt((c1.r - c2.r) * (c1.r - c2.r) +
                (c1.g - c2.g) * (c1.g - c2.g) +
                (c1.b - c2.b) * (c1.b - c2.b));
}

// Функция сравнения для сортировки по каналу R
int compare_r(const void *a, const void *b) {
    return ((RGB*)a)->r - ((RGB*)b)->r;
}

// Функция сравнения для сортировки по каналу G
int compare_g(const void *a, const void *b) {
    return ((RGB*)a)->g - ((RGB*)b)->g;
}

// Функция сравнения для сортировки по каналу B
int compare_b(const void *a, const void *b) {
    return ((RGB*)a)->b - ((RGB*)b)->b;
}

// Функция для разделения цветового пространства
void split_box(ColorBox *box, ColorBox *box1, ColorBox *box2) {
    int i;
    int r_range = 0, g_range = 0, b_range = 0;

    // Находим диапазон с наибольшей разницей
    for (i = 0; i < box->count; i++) {
        r_range += box->colors[i].r;
        g_range += box->colors[i].g;
        b_range += box->colors[i].b;
    }

    r_range /= box->count;
    g_range /= box->count;
    b_range /= box->count;

    int max_range = r_range;
    char channel = 'r';

    if (g_range > max_range) {
        max_range = g_range;
        channel = 'g';
    }
    if (b_range > max_range) {
        max_range = b_range;
        channel = 'b';
    }

    // Сортируем цвета по выбранному каналу
    if (channel == 'r') {
        qsort(box->colors, box->count, sizeof(RGB), compare_r);
    } else if (channel == 'g') {
        qsort(box->colors, box->count, sizeof(RGB), compare_g);
    } else {
        qsort(box->colors, box->count, sizeof(RGB), compare_b);
    }

    // Разделяем на две группы
    int mid = box->count / 2;
    box1->colors = box->colors;
    box1->count = mid;
    box2->colors = box->colors + mid;
    box2->count = box->count - mid;
}

// Функция для создания палитры
void create_palette(RGB *colors, int num_colors, RGB *palette, int palette_size) {
    ColorBox *boxes = (ColorBox *)malloc(palette_size * sizeof(ColorBox));
    boxes[0].colors = colors;
    boxes[0].count = num_colors;

    int box_count = 1;

    while (box_count < palette_size) {
        // Находим коробку с наибольшим диапазоном
        int max_range = 0;
        int max_index = 0;

        for (int i = 0; i < box_count; i++) {
            if (boxes[i].count > max_range) {
                max_range = boxes[i].count;
                max_index = i;
            }
        }

        // Разделяем коробку
        ColorBox box1, box2;
        split_box(&boxes[max_index], &box1, &box2);

        boxes[max_index] = box1;
        boxes[box_count] = box2;
        box_count++;
    }

    // Создаем палитру
    for (int i = 0; i < palette_size; i++) {
        int r = 0, g = 0, b = 0;
        for (int j = 0; j < boxes[i].count; j++) {
            r += boxes[i].colors[j].r;
            g += boxes[i].colors[j].g;
            b += boxes[i].colors[j].b;
        }
        palette[i].r = r / boxes[i].count;
        palette[i].g = g / boxes[i].count;
        palette[i].b = b / boxes[i].count;
    }

    free(boxes);
}

// Функция для преобразования изображения
void convert_to_256_colors(const char *input_file, const char *output_file) {
    FILE *f_in, *f_out;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char *pixels;
    int i, j, padding;

    // Открываем входной файл
    f_in = fopen(input_file, "rb");
    if (f_in == NULL) {
        perror("Ошибка открытия файла");
        return;
    }

    // Чтение заголовков
    fread(&fileHeader, sizeof(fileHeader), 1, f_in);
    fread(&infoHeader, sizeof(infoHeader), 1, f_in);

    // Проверка глубины цвета
    if (infoHeader.biBitCount != 24) {
        printf("Файл должен быть TrueColor BMP!\n");
        fclose(f_in);
        return;
    }

    // Вычисление выравнивания
    padding = (4 - (infoHeader.biWidth * 3) % 4) % 4;

    // Выделение памяти для пикселей
    pixels = (unsigned char *)malloc(infoHeader.biSizeImage);
    fread(pixels, 1, infoHeader.biSizeImage, f_in);
    fclose(f_in);

    // Сбор всех цветов
    RGB *colors = (RGB *)malloc(infoHeader.biWidth * infoHeader.biHeight * sizeof(RGB));
    int num_colors = 0;

    for (i = 0; i < infoHeader.biHeight; i++) {
        for (j = 0; j < infoHeader.biWidth; j++) {
            int index = (i * (infoHeader.biWidth * 3 + padding)) + (j * 3);
            colors[num_colors].r = pixels[index + 2];
            colors[num_colors].g = pixels[index + 1];
            colors[num_colors].b = pixels[index];
            num_colors++;
        }
    }

    // Создание палитры
    RGB palette[256];
    create_palette(colors, num_colors, palette, 256);

    // Замена цветов
    for (i = 0; i < infoHeader.biHeight; i++) {
        for (j = 0; j < infoHeader.biWidth; j++) {
            int index = (i * (infoHeader.biWidth * 3 + padding)) + (j * 3);
            RGB original_color = {pixels[index + 2], pixels[index + 1], pixels[index]};

            // Находим ближайший цвет в палитре
            double min_distance = INFINITY;
            int best_index = 0;

            for (int k = 0; k < 256; k++) {
                double distance = color_distance(original_color, palette[k]);
                if (distance < min_distance) {
                    min_distance = distance;
                    best_index = k;
                }
            }

            // Заменяем цвет
            pixels[index + 2] = palette[best_index].r;
            pixels[index + 1] = palette[best_index].g;
            pixels[index] = palette[best_index].b;
        }
    }

    // Сохранение результата
    f_out = fopen(output_file, "wb");
    if (f_out == NULL) {
        perror("Ошибка создания файла");
        free(pixels);
        free(colors);
        return;
    }

    // Запись заголовков и пикселей
    fwrite(&fileHeader, sizeof(fileHeader), 1, f_out);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f_out);
    fwrite(pixels, 1, infoHeader.biSizeImage, f_out);

    fclose(f_out);
    free(pixels);
    free(colors);

    printf("Изображение преобразовано и сохранено в %s\n", output_file);
}

// Функция для отображения двух BMP файлов на экране
void display_two_bmp(const char *filename1, const char *filename2) {
    FILE *f1, *f2;
    BITMAPFILEHEADER fileHeader1, fileHeader2;
    BITMAPINFOHEADER infoHeader1, infoHeader2;
    unsigned char *pixels1, *pixels2;
    int i, j, padding1, padding2;

    // Открываем первый BMP файл
    f1 = fopen(filename1, "rb");
    if (f1 == NULL) {
        perror("Ошибка открытия файла");
        return;
    }

    // Чтение заголовков первого файла
    fread(&fileHeader1, sizeof(fileHeader1), 1, f1);
    fread(&infoHeader1, sizeof(infoHeader1), 1, f1);

    // Проверка глубины цвета
    if (infoHeader1.biBitCount != 24) {
        printf("Файл должен быть TrueColor BMP!\n");
        fclose(f1);
        return;
    }

    // Вычисление выравнивания
    padding1 = (4 - (infoHeader1.biWidth * 3) % 4) % 4;

    // Выделение памяти для пикселей первого файла
    pixels1 = (unsigned char *)malloc(infoHeader1.biSizeImage);
    fread(pixels1, 1, infoHeader1.biSizeImage, f1);
    fclose(f1);

    // Открываем второй BMP файл
    f2 = fopen(filename2, "rb");
    if (f2 == NULL) {
        perror("Ошибка открытия файла");
        free(pixels1);
        return;
    }

    // Чтение заголовков второго файла
    fread(&fileHeader2, sizeof(fileHeader2), 1, f2);
    fread(&infoHeader2, sizeof(infoHeader2), 1, f2);

    // Проверка глубины цвета
    if (infoHeader2.biBitCount != 24) {
        printf("Файл должен быть TrueColor BMP!\n");
        fclose(f2);
        free(pixels1);
        return;
    }

    // Вычисление выравнивания
    padding2 = (4 - (infoHeader2.biWidth * 3) % 4) % 4;

    // Выделение памяти для пикселей второго файла
    pixels2 = (unsigned char *)malloc(infoHeader2.biSizeImage);
    fread(pixels2, 1, infoHeader2.biSizeImage, f2);
    fclose(f2);

    // Инициализация X11
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        printf("Ошибка подключения к X серверу!\n");
        free(pixels1);
        free(pixels2);
        return;
    }

    int screen = DefaultScreen(display);
    Window window = XCreateSimpleWindow(
        display, RootWindow(display, screen),
        0, 0, infoHeader1.biWidth, infoHeader1.biHeight + infoHeader2.biHeight, 1,
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

    // Отображение первого изображения (сверху)
    for (i = 0; i < infoHeader1.biHeight; i++) {
        for (j = 0; j < infoHeader1.biWidth; j++) {
            int index = (i * (infoHeader1.biWidth * 3 + padding1)) + (j * 3);
            unsigned char r = pixels1[index + 2];
            unsigned char g = pixels1[index + 1];
            unsigned char b = pixels1[index];
            XSetForeground(display, gc, (r << 16) | (g << 8) | b);
            XDrawPoint(display, window, gc, j, infoHeader1.biHeight - i - 1);
        }
    }

    // Отображение второго изображения (снизу)
    for (i = 0; i < infoHeader2.biHeight; i++) {
        for (j = 0; j < infoHeader2.biWidth; j++) {
            int index = (i * (infoHeader2.biWidth * 3 + padding2)) + (j * 3);
            unsigned char r = pixels2[index + 2];
            unsigned char g = pixels2[index + 1];
            unsigned char b = pixels2[index];
            XSetForeground(display, gc, (r << 16) | (g << 8) | b);
            XDrawPoint(display, window, gc, j, infoHeader1.biHeight + infoHeader2.biHeight - i - 1);
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
    free(pixels1);
    free(pixels2);
}

int main() {
    // Преобразование изображения
    convert_to_256_colors("carib.bmp", "output_256.bmp");

    // Отображение исходного и преобразованного изображения
    printf("Отображение исходного и преобразованного изображения...\n");
    display_two_bmp("carib.bmp", "output_256.bmp");

    return 0;
}