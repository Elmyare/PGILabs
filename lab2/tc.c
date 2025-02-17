#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

void add_border_24bit(const char *input_file, const char *output_file, int border_width) {
    FILE *f1, *f2;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char *pixels, *new_pixels;
    int i, j, row, new_row, padding, new_padding;

    // Открываем исходный файл
    f1 = fopen(input_file, "rb");
    if (f1 == NULL) {
        perror("Ошибка открытия файла");
        return;
    }

    // Читаем заголовки
    fread(&fileHeader, sizeof(fileHeader), 1, f1);
    fread(&infoHeader, sizeof(infoHeader), 1, f1);

    // Проверяем, что это 24-битный BMP
    if (infoHeader.biBitCount != 24) {
        printf("Файл должен иметь глубину цвета 24 бита!\n");
        fclose(f1);
        return;
    }

    // Вычисляем выравнивание для исходного изображения
    padding = (4 - ((infoHeader.biWidth * 3) % 4)) % 4;

    // Выделяем память для пикселей
    pixels = (unsigned char *)malloc(infoHeader.biWidth * infoHeader.biHeight * 3);
    fread(pixels, 1, infoHeader.biWidth * infoHeader.biHeight * 3, f1);

    // Закрываем исходный файл
    fclose(f1);

    // Новые размеры изображения
    int new_width = infoHeader.biWidth + 2 * border_width;
    int new_height = infoHeader.biHeight + 2 * border_width;

    // Вычисляем выравнивание для нового изображения
    new_padding = (4 - ((new_width * 3) % 4)) % 4;

    // Выделяем память для нового изображения
    new_pixels = (unsigned char *)malloc(new_width * new_height * 3);

    // Заполняем рамку случайными цветами
    srand(time(NULL));
    for (i = 0; i < new_height; i++) {
        for (j = 0; j < new_width; j++) {
            int index = (i * new_width + j) * 3;
            if (i < border_width || i >= new_height - border_width ||
                j < border_width || j >= new_width - border_width) {
                new_pixels[index] = rand() % 256;     // Синий
                new_pixels[index + 1] = rand() % 256; // Зеленый
                new_pixels[index + 2] = rand() % 256; // Красный
            } else {
                int original_i = i - border_width;
                int original_j = j - border_width;
                int original_index = (original_i * infoHeader.biWidth + original_j) * 3;
                new_pixels[index] = pixels[original_index];
                new_pixels[index + 1] = pixels[original_index + 1];
                new_pixels[index + 2] = pixels[original_index + 2];
            }
        }
    }

    // Обновляем заголовок
    infoHeader.biWidth = new_width;
    infoHeader.biHeight = new_height;
    infoHeader.biSizeImage = (new_width * 3 + new_padding) * new_height;
    fileHeader.bfSize = sizeof(fileHeader) + sizeof(infoHeader) + infoHeader.biSizeImage;

    // Открываем новый файл для записи
    f2 = fopen(output_file, "wb");
    if (f2 == NULL) {
        perror("Ошибка создания файла");
        free(pixels);
        free(new_pixels);
        return;
    }

    // Записываем заголовки
    fwrite(&fileHeader, sizeof(fileHeader), 1, f2);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f2);

    // Записываем пиксельные данные с выравниванием
    for (i = 0; i < new_height; i++) {
        fwrite(new_pixels + i * new_width * 3, 1, new_width * 3, f2);
        for (j = 0; j < new_padding; j++) {
            fputc(0, f2); // Выравнивающие байты
        }
    }

    // Закрываем файл и освобождаем память
    fclose(f2);
    free(pixels);
    free(new_pixels);

    printf("Рамка добавлена. Результат сохранен в %s\n", output_file);
}

int main() {
    add_border_24bit("carib.bmp", "caribTC_bordered.bmp", 15);
    return 0;
}