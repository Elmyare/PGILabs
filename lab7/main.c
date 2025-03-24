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

void embed_text(const char *bmp_file, const char *text_file, const char *output_file, int bits) {
    FILE *f_bmp, *f_text, *f_out;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char *pixels;

    f_bmp = fopen(bmp_file, "rb");
    if (!f_bmp) {
        perror("Ошибка открытия BMP файла");
        return;
    }

    fread(&fileHeader, sizeof(fileHeader), 1, f_bmp);
    fread(&infoHeader, sizeof(infoHeader), 1, f_bmp);

    if (infoHeader.biBitCount != 24) {
        printf("Файл должен быть TrueColor BMP!\n");
        fclose(f_bmp);
        return;
    }

    pixels = (unsigned char *)malloc(infoHeader.biSizeImage);
    fread(pixels, 1, infoHeader.biSizeImage, f_bmp);
    fclose(f_bmp);

    f_text = fopen(text_file, "rb");
    if (!f_text) {
        perror("Ошибка открытия текстового файла");
        free(pixels);
        return;
    }

    fseek(f_text, 0, SEEK_END);
    long text_size = ftell(f_text);
    fseek(f_text, 0, SEEK_SET);

    long max_text_size = (infoHeader.biSizeImage * bits) / 8;
    if (text_size > max_text_size) {
        printf("Текст слишком большой! (%ld > %ld)\n", text_size, max_text_size);
        fclose(f_text);
        free(pixels);
        return;
    }

    unsigned char *text_data = (unsigned char *)calloc(max_text_size, 1);
    fread(text_data, 1, text_size, f_text);
    fclose(f_text);

    int bit_pos = 0;
    for (int i = 0; i < infoHeader.biSizeImage && bit_pos / 8 < text_size; i++) {
        for (int b = 0; b < bits; b++) {
            unsigned char bit = (text_data[bit_pos / 8] >> (bit_pos % 8)) & 1;
            pixels[i] = (pixels[i] & ~(1 << b)) | (bit << b);
            bit_pos++;
        }
    }

    f_out = fopen(output_file, "wb");
    fwrite(&fileHeader, sizeof(fileHeader), 1, f_out);
    fwrite(&infoHeader, sizeof(infoHeader), 1, f_out);
    fwrite(pixels, 1, infoHeader.biSizeImage, f_out);
    fclose(f_out);

    free(pixels);
    free(text_data);

    printf("Текст внедрен в %s\n", output_file);
}

void extract_text(const char *bmp_file, const char *output_file, int bits, long text_size) {
    FILE *f_bmp, *f_out;
    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;
    unsigned char *pixels;

    f_bmp = fopen(bmp_file, "rb");
    if (!f_bmp) {
        perror("Ошибка открытия BMP файла");
        return;
    }

    fread(&fileHeader, sizeof(fileHeader), 1, f_bmp);
    fread(&infoHeader, sizeof(infoHeader), 1, f_bmp);

    if (infoHeader.biBitCount != 24) {
        printf("Файл должен быть TrueColor BMP!\n");
        fclose(f_bmp);
        return;
    }

    pixels = (unsigned char *)malloc(infoHeader.biSizeImage);
    fread(pixels, 1, infoHeader.biSizeImage, f_bmp);
    fclose(f_bmp);

    f_out = fopen(output_file, "wb");
    if (!f_out) {
        perror("Ошибка создания файла");
        free(pixels);
        return;
    }

    unsigned char *text_data = (unsigned char *)calloc(text_size, 1);
    int bit_pos = 0;

    for (int i = 0; i < infoHeader.biSizeImage && bit_pos / 8 < text_size; i++) {
        for (int b = 0; b < bits; b++) {
            unsigned char bit = (pixels[i] >> b) & 1;
            text_data[bit_pos / 8] |= bit << (bit_pos % 8);
            if ((bit_pos % 8) + bits > 8) {
                text_data[bit_pos / 8 + 1] |= bit >> (8 - (bit_pos % 8));
            }
            bit_pos++;
        }
    }

    fwrite(text_data, 1, text_size, f_out);

    fclose(f_out);
    free(pixels);
    free(text_data);

    printf("Текст извлечен в %s\n", output_file);
}

int main() {
    embed_text("carib.bmp", "secret25.txt", "output25.bmp", 2);
    embed_text("carib.bmp", "secret50.txt", "output50.bmp", 4);
    embed_text("carib.bmp", "secret75.txt", "output75.bmp", 6);

    extract_text("output25.bmp", "extracted25.txt", 2, 360000);
    extract_text("output50.bmp", "extracted50.txt", 4, 720000);
    extract_text("output75.bmp", "extracted75.txt", 6, 1080000);

    return 0;
}
