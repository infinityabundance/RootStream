/*
 * qrcode.c - QR code generation for RootStream codes
 * 
 * Uses qrencode library to generate QR codes that can be:
 * - Displayed in GTK window
 * - Saved as PNG
 * - Printed to terminal (ASCII art)
 * 
 * QR code contains the full RootStream code:
 *   base64_pubkey@hostname
 * 
 * Scanning this QR code on another device allows instant pairing.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* libqrencode for QR code generation */
#include <qrencode.h>

/* For PNG output */
#include <png.h>

/*
 * Generate QR code and save as PNG
 * 
 * @param data        Data to encode (RootStream code)
 * @param output_file Output PNG file path
 * @return            0 on success, -1 on error
 * 
 * QR code settings:
 * - Version: auto (adaptive)
 * - Error correction: Medium (15% recovery)
 * - Size: 8 pixels per module (readable on screen)
 */
int qrcode_generate(const char *data, const char *output_file) {
    if (!data || !output_file) {
        fprintf(stderr, "ERROR: Invalid arguments to qrcode_generate\n");
        return -1;
    }

    /* Generate QR code */
    QRcode *qr = QRcode_encodeString(data, 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!qr) {
        fprintf(stderr, "ERROR: Failed to generate QR code\n");
        fprintf(stderr, "DATA: %s\n", data);
        return -1;
    }

    /* Calculate image size (8 pixels per module + 4 module border) */
    int scale = 8;
    int border = 4;
    int size = (qr->width + border * 2) * scale;

    /* Create PNG */
    FILE *fp = fopen(output_file, "wb");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create output file\n");
        fprintf(stderr, "FILE: %s\n", output_file);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        QRcode_free(qr);
        return -1;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 
                                              NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        QRcode_free(qr);
        return -1;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        QRcode_free(qr);
        return -1;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        QRcode_free(qr);
        return -1;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, size, size, 8, PNG_COLOR_TYPE_GRAY,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    /* Allocate row buffer */
    png_bytep row = malloc(size);
    if (!row) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        QRcode_free(qr);
        return -1;
    }

    /* Write QR code pixels */
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int qr_x = x / scale - border;
            int qr_y = y / scale - border;

            /* White border */
            if (qr_x < 0 || qr_y < 0 || qr_x >= qr->width || qr_y >= qr->width) {
                row[x] = 255;  /* White */
            } else {
                /* Black if module is set, white otherwise */
                int index = qr_y * qr->width + qr_x;
                row[x] = (qr->data[index] & 1) ? 0 : 255;
            }
        }
        png_write_row(png, row);
    }

    png_write_end(png, NULL);

    /* Cleanup */
    free(row);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    QRcode_free(qr);

    printf("✓ Generated QR code: %s\n", output_file);

    return 0;
}

/*
 * Print QR code to terminal (ASCII art)
 * 
 * @param data RootStream code to encode
 * 
 * Uses Unicode block characters for better visual:
 * - █ (full block) for black modules
 * - ' ' (space) for white modules
 */
void qrcode_print_terminal(const char *data) {
    if (!data) return;

    QRcode *qr = QRcode_encodeString(data, 0, QR_ECLEVEL_M, QR_MODE_8, 1);
    if (!qr) {
        fprintf(stderr, "ERROR: Failed to generate QR code\n");
        return;
    }

    printf("\n");
    printf("╔");
    for (int i = 0; i < qr->width + 2; i++) printf("═");
    printf("╗\n");

    for (int y = 0; y < qr->width; y++) {
        printf("║ ");
        for (int x = 0; x < qr->width; x++) {
            int index = y * qr->width + x;
            printf("%s", (qr->data[index] & 1) ? "██" : "  ");
        }
        printf(" ║\n");
    }

    printf("╚");
    for (int i = 0; i < qr->width + 2; i++) printf("═");
    printf("╝\n");

    printf("\nRootStream Code: %s\n\n", data);

    QRcode_free(qr);
}

/*
 * Display QR code in GTK window
 * 
 * @param ctx  RootStream context
 * @param code RootStream code to display
 * @return     0 on success, -1 on error
 * 
 * Creates a simple GTK window showing:
 * - QR code image
 * - RootStream code text
 * - Copy button
 */
int qrcode_display(rootstream_ctx_t *ctx, const char *code) {
    if (!ctx || !code) {
        return -1;
    }

    /* Generate QR code PNG in temp file */
    char qr_path[256];
    snprintf(qr_path, sizeof(qr_path), "/tmp/rootstream-qr-%d.png", getpid());

    if (qrcode_generate(code, qr_path) < 0) {
        return -1;
    }

    /* TODO: Display in GTK window (see tray.c) */
    /* For now, just print to terminal */
    qrcode_print_terminal(code);

    return 0;
}
