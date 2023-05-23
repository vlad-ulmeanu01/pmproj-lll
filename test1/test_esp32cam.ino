#include "esp_camera.h"

#include "compresser.h"
#include "util.h"

uint8_t *im, *quad_help_min, *quad_help_max;
compresser::CompressedImage cim;

void setup() {
    Serial.begin(115200);

    // Initialize the camera
    camera_config_t camera_config;
  
    camera_config.pin_pwdn = CAM_PIN_PWDN;
    camera_config.pin_reset = CAM_PIN_RESET;
    camera_config.pin_xclk = CAM_PIN_XCLK;
    camera_config.pin_sscb_sda = CAM_PIN_SIOD;
    camera_config.pin_sscb_scl = CAM_PIN_SIOC;
    camera_config.pin_d7 = CAM_PIN_D7;
    camera_config.pin_d6 = CAM_PIN_D6;
    camera_config.pin_d5 = CAM_PIN_D5;
    camera_config.pin_d4 = CAM_PIN_D4;
    camera_config.pin_d3 = CAM_PIN_D3;
    camera_config.pin_d2 = CAM_PIN_D2;
    camera_config.pin_vsync = CAM_PIN_VSYNC;
    camera_config.pin_href = CAM_PIN_HREF;
    camera_config.pin_pclk = CAM_PIN_PCLK;
    camera_config.xclk_freq_hz = 20000000;
    camera_config.ledc_timer = LEDC_TIMER_0;
    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.pixel_format = PIXFORMAT_RGB565; //YUV422, GRAYSCALE, RGB565, JPEG.
    camera_config.frame_size = FRAMESIZE_240X240; ///QVGA este 320x240. daca s-ar putea, as fi pus 256x256.
    camera_config.jpeg_quality = 9; ///apartine [0, 63]. numar mai mic = calitate mai buna.
    camera_config.fb_count = 1;
  
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return;
    }

    Serial.println("Camera initialized successfully");

    im = (uint8_t *)ps_malloc(BUFF888_SIZE);
    assert(im); Serial.println("im ok");

    cim.buff = (uint8_t *)ps_malloc(COMPRESSED_MAX_SIZE);
    assert(cim.buff); Serial.println("cim.buff ok");
    compresser::lazy_reset(&cim);

    quad_help_min = (uint8_t *)ps_malloc(QUAD_TREE_SIZE);
    assert(quad_help_min); Serial.println("quad_help_min ok");

    quad_help_max = (uint8_t *)ps_malloc(QUAD_TREE_SIZE);
    assert(quad_help_max); Serial.println("quad_help_max ok");

    Serial.println("Alloc ok.");

    // Capture an image
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    // Image is stored in 'fb->buf' with a size of 'fb->len' bytes
    // You can now use the image data in your application
    util::convertA(fb->buf, im);
    
    //memset(im, 0xE2, BUFF888_SIZE); ///dbg: ar tb sa fie doar 4 pixeli dc imaginea e complet neagra.
    //for (int i = 0, j = 0; i < 256; i += 8, j++) memset(im + j * 6144, i, 6144);
    //memset(im, 0, BUFF888_SIZE); memset(im, 255, 6144);

    // Return the frame buffer to the driver for reuse
    esp_camera_fb_return(fb);

    compresser::compress(&cim, im, quad_help_min, quad_help_max);
    Serial.printf("cim.len = %d\n", cim.len);

    //START_PICS

    //Serial.write(cim.buff, cim.len + 1);
    for (int i = 0; i <= cim.len; i++) {
        Serial.printf("%d\n", cim.buff[i]);
    }
    
    free(quad_help_max);
    free(quad_help_min);
    free(cim.buff);
    free(im);
}

void loop() {
    delay(1000);
}
