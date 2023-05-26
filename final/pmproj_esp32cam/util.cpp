#include "util.h"

namespace util {
    void convertA(uint8_t *buff, uint8_t *im) {
        ///buff este 240x240, RGB565.
        ///im este 256x256, RGB888.
        ///pixelii in plus (dreapta/jos) sunt negri.

        int buff_size = 240 * 240 * 2, im_size = IMG_LEN * IMG_LEN * 3, i, j, z, x, y;

        memset(im, 0, im_size);

        uint8_t r, g, b;
        for (i = 0, j = 0, x = 0, y = 0; i < buff_size; i += 2) {
            r = (buff[i] & 0b11111000);
            g = ((buff[i] & 0b00000111) << 5) | ((buff[i+1] & 0b11100000) >> 3);
            b = ((buff[i+1] & 0b00011111) << 3);

            //j = y * IMG_LEN + x;
            z = 3 * j;
            im[z] = r;
            im[z+1] = g;
            im[z+2] = b;

            j++;
            x++;
            if (x == 240) {
                j -= 240;
                j += IMG_LEN;
                x = 0;
                y++;
            }
        }
    }
}
