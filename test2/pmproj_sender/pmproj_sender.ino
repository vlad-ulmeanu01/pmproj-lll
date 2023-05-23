///Sender, ESP32-WROOMU
#include <HardwareSerial.h>

HardwareSerial CustomPort(2); // use UART2

#define START_BYTE 0xE2
#define CONSECUTIVE_NEEDED_START_BYTES 32
#define START_PICS(_b) for (int _ = 0; _ < CONSECUTIVE_NEEDED_START_BYTES; _++) Serial.write(_b);

#define STATE_IDLE 0 ///nu am comandat inca primirea unei imagini de la ESP32-CAM.
#define STATE_GET_SIZE 1 ///am comandat primirea unei imagini si astept primii 4 bytes, care imi spun marimea imaginii.
#define STATE_GET_BYTES 2 ///inca astept sa primesc toti bytes-ii din imagine.

uint8_t ch;
int state = 0;
int remainingImageBytes = 0, receivedSizeBytes = 0;

void setup() {
    Serial.begin(115200);
    CustomPort.begin(115200, SERIAL_8N1, 16, 17); ///RX, TX.
    if(!CustomPort) {
        Serial.printf("CustomPort failed to start..\n");
    } else {
        Serial.printf("CustomPort started!\n");
    }

    bool canStart = false;
    int consecutiveStartBytes = 0;

    while (!canStart) {
        while (CustomPort.available() > 0 && consecutiveStartBytes < CONSECUTIVE_NEEDED_START_BYTES) {
            ch = CustomPort.read();
            if (ch == START_BYTE) {
                consecutiveStartBytes++;
            } else {
                consecutiveStartBytes = 0;
            }

            //Serial.printf("Read ch = %d, consecutiveStartBytes = %d\n", ch, consecutiveStartBytes);
            //Serial.write(ch);
        }

        if (consecutiveStartBytes >= CONSECUTIVE_NEEDED_START_BYTES) {
            canStart = true;
        }
    }

    Serial.printf("Will start receiving images.\n");
    START_PICS(START_BYTE); ///anunt si laptop-ul ca o sa primeasca imagini.
}

void loop() {
    if (state == STATE_IDLE) {
        CustomPort.write(0); ///doar scriu un caracter.
        remainingImageBytes = 0;
        receivedSizeBytes = 0;
        state = STATE_GET_SIZE;
    } else if (state == STATE_GET_SIZE) {
        while (CustomPort.available() > 0 && receivedSizeBytes < 4) {
            ch = CustomPort.read();
            Serial.write(ch);
            receivedSizeBytes++;
            remainingImageBytes = (remainingImageBytes << 8) | ch;
        }

        if (receivedSizeBytes >= 4) {
            //Serial.printf("(dbg) remainingImageBytes = %d\n", remainingImageBytes);
            state = STATE_GET_BYTES;
        }
    } else { ///STATE_GET_BYTES.
        while (CustomPort.available() > 0 && remainingImageBytes > 0) {
            ch = CustomPort.read();
            Serial.write(ch);
            remainingImageBytes--;
        }

        if (remainingImageBytes <= 0) {
            state = STATE_IDLE;
            delay(50);
        }
    }
}
