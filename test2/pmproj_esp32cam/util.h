#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>
#include <string.h>

// Camera pins for your ESP32-CAM module
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#define IMG_LEN 256
#define BUFF565_SIZE 131072 ///256x256 pixeli. (RGB565 format, 2B per pixel).
#define BUFF888_SIZE 196608 ///256x256 pixeli. (RGB888 format, 3B per pixel).
#define QUAD_TREE_SIZE 262143 ///(4^0+..+4^8) * 3 = 4^9 - 1.
#define COMPRESSED_MAX_SIZE 286720 ///256x256 pixeli compresati, maxim 35 biti bucata. nu ar trebui niciodata sa ma aproprii de nr asta.

///constante compresie.
#define MAX_ARIE 65536 ///pentru 256x256 pixeli.
#define DISP_THRESH 115
#define SIGMOID_COEF 0.00390625f

#define START_PICS(_b) for (int _ = 0; _ < 32; _++) Serial.write(_b);

namespace util {
    ///converteste un buffer cu o imagine 240x240 RGB565 intr-un buffer 256x256 RGB888.
    void convertA(uint8_t *buff, uint8_t *im);

    ///pui log4(arie) si scoti sigmoid(arie * SIGMOID_COEF).
    ///pentru arie = 1: ..[0], arie = 4: ..[1], ... arie = MAX_ARIE: ..[8].
    ///cu cat e aria mai mica, cu atat sunt mai dispus sa compresez si salut.

    ///incercare folosire tot intervalul [0, 1] inl doar [0.5, 1].
    //static float coef_corector_lookup[9] = {0.0039f, 0.0156f, 0.0624f, 0.2449f, 0.7616f, 0.9993f, 1.0f, 1.0f, 1.0f}; //0.0078125f
    static float coef_corector_lookup[9] = {0.501f, 0.5039f, 0.5156f, 0.5622f, 0.7311f, 0.982f, 1.0f, 1.0f, 1.0f}; //0.00390625f
}

#endif
