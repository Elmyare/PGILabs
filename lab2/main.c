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
#pragma pack(pop)

void add_border_8bit(const char *input_file, const char *output_file, int border_width) {
    FILE *f1, *f2;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char palitra[256][4];
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

    // Проверяем, что это 8-битный BMP
    if (infoHeader.biBitCount != 8) {
        printf("Файл должен иметь глубину цвета 8 бит!\n");
        fclose(f1);
        return;
    }

    // Читаем палитру
    fread(palitra, 4, 256, f1);

    // Вычисляем выравнивание для исходного изображения
    padding = (4 - (infoHeader.biWidth % 4)) % 4;

    // Выделяем память для пикселей
    pixels = (unsigned char *)malloc(infoHeader.biWidth * infoHeader.biHeight);
    if (pixels == NULL) {
        printf("Ошибка выделения памяти!\n");
        fclose(f1);
        return;
    }

    // Читаем пиксельные данные с учетом выравнивания
    for (i = 0; i < infoHeader.biHeight; i++) {
        fread(pixels + i * infoHeader.biWidth, 1, infoHeader.biWidth, f1);
        fseek(f1, padding, SEEK_CUR); // Пропускаем выравнивающие байты
    }

    // Закрываем исходный файл
    fclose(f1);

    // Новые размеры изображения
    int new_width = infoHeader.biWidth + 2 * border_width;
    int new_height = infoHeader.biHeight + 2 * border_width;

    // Вычисляем выравнивание для нового изображения
    new_padding = (4 - (new_width % 4)) % 4;

    // Выделяем память для нового изображения
    new_pixels = (unsigned char *)malloc(new_width * new_height);
    if (new_pixels == NULL) {
        printf("Ошибка выделения памяти!\n");
        free(pixels);
        return;
    }

    // Заполняем рамку случайными цветами
    srand(time(NULL));
    for (i = 0; i < new_height; i++) {
        for (j = 0; j < new_width; j++) {
            if (i < border_width || i >= new_height - border_width ||
                j < border_width || j >= new_width - border_width) {
                new_pixels[i * new_width + j] = rand() % 256; // Случайный цвет из палитры
            } else {
                int original_i = i - border_width;
                int original_j = j - border_width;
                new_pixels[i * new_width + j] = pixels[original_i * infoHeader.biWidth + original_j];
            }
        }
    }

    // Обновляем заголовок
    infoHeader.biWidth = new_width;
    infoHeader.biHeight = new_height;
    infoHeader.biSizeImage = (new_width + new_padding) * new_height;
    fileHeader.bfSize = sizeof(fileHeader) + sizeof(infoHeader) + 256 * 4 + infoHeader.biSizeImage;

    // Открываем новый файл для записи
    f2 = fopen(output_file, "wb");
    if (f2 == NULL) {
        perror("Ошибка создания файла");
        free(pixels);
        free(new_pixels);
        return;
    }

    // Записываем заголовки и палитру
    fwrite(&fileHeader, sizeof(fileHeader), 1, f2);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f2);
    fwrite(palitra, 4, 256, f2);

    // Записываем пиксельные данные с выравниванием
    for (i = 0; i < new_height; i++) {
        fwrite(new_pixels + i * new_width, 1, new_width, f2);
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
    add_border_8bit("CAT256.bmp", "cat256_bordered.bmp", 15);
    return 0;
}