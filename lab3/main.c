#include <stdio.h>
#include <stdlib.h>

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

void rotate_bmp_90(const char *input_file, const char *output_file) {
    FILE *f1, *f2;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char *pixels, *rotated_pixels;
    int i, j, padding, new_padding;

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
    if (pixels == NULL) {
        printf("Ошибка выделения памяти!\n");
        fclose(f1);
        return;
    }

    // Читаем пиксельные данные с учетом выравнивания
    for (i = 0; i < infoHeader.biHeight; i++) {
        fread(pixels + i * infoHeader.biWidth * 3, 1, infoHeader.biWidth * 3, f1);
        fseek(f1, padding, SEEK_CUR); // Пропускаем выравнивающие байты
    }

    // Закрываем исходный файл
    fclose(f1);

    // Новые размеры изображения (ширина и высота меняются местами)
    int new_width = infoHeader.biHeight;
    int new_height = infoHeader.biWidth;

    // Вычисляем выравнивание для нового изображения
    new_padding = (4 - ((new_width * 3) % 4)) % 4;

    // Выделяем память для повернутого изображения
    rotated_pixels = (unsigned char *)malloc(new_width * new_height * 3);
    if (rotated_pixels == NULL) {
        printf("Ошибка выделения памяти!\n");
        free(pixels);
        return;
    }

    // Разворот изображения на 90 градусов по часовой стрелке
    for (i = 0; i < infoHeader.biHeight; i++) {
        for (j = 0; j < infoHeader.biWidth; j++) {
            int original_index = (i * infoHeader.biWidth + j) * 3;
            int new_index = ((new_height - j - 1) * new_width + i) * 3;
            rotated_pixels[new_index] = pixels[original_index];         // Синий
            rotated_pixels[new_index + 1] = pixels[original_index + 1]; // Зеленый
            rotated_pixels[new_index + 2] = pixels[original_index + 2]; // Красный
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
        free(rotated_pixels);
        return;
    }

    // Записываем заголовки
    fwrite(&fileHeader, sizeof(fileHeader), 1, f2);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f2);

    // Записываем пиксельные данные с выравниванием
    for (i = 0; i < new_height; i++) {
        fwrite(rotated_pixels + i * new_width * 3, 1, new_width * 3, f2);
        for (j = 0; j < new_padding; j++) {
            fputc(0, f2); // Выравнивающие байты
        }
    }

    // Закрываем файл и освобождаем память
    fclose(f2);
    free(pixels);
    free(rotated_pixels);

    printf("Изображение повернуто на 90 градусов. Результат сохранен в %s\n", output_file);
}

int main() {
    rotate_bmp_90("carib.bmp", "carib_rotated.bmp");
    return 0;
}