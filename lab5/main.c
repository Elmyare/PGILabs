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

void scale_bmp(const char *input_file, const char *output_file, float scale) {
    FILE *f1, *f2;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char palette[256][4];
    unsigned char *pixels, *scaledPixels;
    int i, j, padding, newPadding;

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
    fread(palette, 4, 256, f1);

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
    int new_width = infoHeader.biWidth * scale;
    int new_height = infoHeader.biHeight * scale;

    // Вычисляем выравнивание для нового изображения
    newPadding = (4 - (new_width % 4)) % 4;

    // Выделяем память для нового изображения
    scaledPixels = (unsigned char *)malloc(new_width * new_height);
    if (scaledPixels == NULL) {
        printf("Ошибка выделения памяти!\n");
        free(pixels);
        return;
    }

    // Масштабирование изображения
    for (i = 0; i < new_height; i++) {
        for (j = 0; j < new_width; j++) {
            int original_i = i / scale;
            int original_j = j / scale;
            scaledPixels[i * new_width + j] = pixels[original_i * infoHeader.biWidth + original_j];
        }
    }

    // Обновляем заголовок
    infoHeader.biWidth = new_width;
    infoHeader.biHeight = new_height;
    infoHeader.biSizeImage = (new_width + newPadding) * new_height;
    fileHeader.bfSize = sizeof(fileHeader) + sizeof(infoHeader) + 256 * 4 + infoHeader.biSizeImage;

    // Открываем новый файл для записи
    f2 = fopen(output_file, "wb");
    if (f2 == NULL) {
        perror("Ошибка создания файла");
        free(pixels);
        free(scaledPixels);
        return;
    }

    // Записываем заголовки и палитру
    fwrite(&fileHeader, sizeof(fileHeader), 1, f2);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f2);
    fwrite(palette, 4, 256, f2);

    // Записываем пиксельные данные с выравниванием
    for (i = 0; i < new_height; i++) {
        fwrite(scaledPixels + i * new_width, 1, new_width, f2);
        for (j = 0; j < newPadding; j++) {
            fputc(0, f2); // Выравнивающие байты
        }
    }

    // Закрываем файл и освобождаем память
    fclose(f2);
    free(pixels);
    free(scaledPixels);

    printf("Масштабирование завершено. Результат сохранен в %s\n", output_file);
}

int main() {
    scale_bmp("CAT256.BMP", "scaled_CAT256.BMP", 0.4); // Масштабирование в 2 раза
    return 0;
}