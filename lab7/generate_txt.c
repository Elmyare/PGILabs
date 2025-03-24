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

void generate_text_file(const char *filename, long size) {
    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        perror("Ошибка создания файла");
        return;
    }

    // Генерация случайных символов
    for (long i = 0; i < size; i++) {
        char c = rand() % 26 + 97; // Случайные символы от 'a' до 'z'
        fputc(c, f);
    }

    fclose(f);
    printf("Текстовый файл %s создан. Размер: %ld байт\n", filename, size);
}

int main() {
    srand(time(NULL));

    const char *bmp_file = "carib.bmp";
    FILE *f_bmp = fopen(bmp_file, "rb");
    if (f_bmp == NULL) {
        perror("Ошибка открытия BMP файла");
        return 1;
    }

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // Чтение заголовков
    fread(&fileHeader, sizeof(fileHeader), 1, f_bmp);
    fread(&infoHeader, sizeof(infoHeader), 1, f_bmp);

    // Проверяем, что это TrueColor BMP
    if (infoHeader.biBitCount != 24) {
        printf("Файл должен быть TrueColor BMP!\n");
        fclose(f_bmp);
        return 1;
    }

    // Вычисляем паддинг
    int padding = (4 - (infoHeader.biWidth * 3) % 4) % 4;

    // Размер графических данных без учета паддинга
    long image_data_size = infoHeader.biHeight * infoHeader.biWidth * 3;
    printf("Размер графических данных (без паддинга): %ld байт\n", image_data_size);

    // Закрываем BMP файл
    fclose(f_bmp);

    // Генерация текстовых файлов
    generate_text_file("secret25.txt", image_data_size * 0.25);
    generate_text_file("secret50.txt", image_data_size * 0.50);
    generate_text_file("secret75.txt", image_data_size * 0.75);

    return 0;
}