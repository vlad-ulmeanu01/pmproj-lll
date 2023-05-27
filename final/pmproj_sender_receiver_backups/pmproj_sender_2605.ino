///Sender, ESP32-WROOMU
#include <SPI.h>
#include <LoRa.h>
#include <HardwareSerial.h>

HardwareSerial CustomPort(2); // use UART2

#define START_BYTE 0xE2
#define CONSECUTIVE_NEEDED_START_BYTES 32
#define START_PICS(_b) for (int _ = 0; _ < CONSECUTIVE_NEEDED_START_BYTES; _++) Serial.write(_b);

#define STATE_IDLE 0 ///nu am comandat inca primirea unei imagini de la ESP32-CAM.
#define STATE_GET_SIZE 1 ///am comandat primirea unei imagini si astept primii 4 bytes, care imi spun marimea imaginii.
#define STATE_GET_BYTES 2 ///inca astept sa primesc toti bytes-ii din imagine. ii pun intr-un buffer. nu vreau sa folosesc si lora in acelasi timp.
#define STATE_SEND_LORA 3 ///transmit bytes-ii din buffer.
#define STATE_WAIT_TX 4 ///astept sa fi trimis mesajul.
#define STATE_WAIT_ACK 5 ///am trimis niste bytes, astept ack.

#define LORA_PACKET_SIZE 220 ///payload-ul are maxim 220 bytes per pachet.
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

#define LIMITED_IMBUFF_MAX_SIZE 65536

uint8_t *limited_imbuff;
int limitedImbuffSize = 0, limitedImbuffIndex = 0, limitedImbuffPrevIndex = 0;
int remainingImageBytes = 0, receivedSizeBytes = 0;

volatile int packetSize = 0, state = STATE_IDLE;
volatile bool justReceivedAck = false, isCrcOk = true;

void onReceive(int psz) {
    packetSize = psz;
    justReceivedAck = true;
    isCrcOk = true;
}

void onTxDone() {
    state = STATE_WAIT_ACK;
    LoRa.receive();
}

void onCrcError() {
    justReceivedAck = true;
    isCrcOk = false;
}

void setup() {
    Serial.begin(115200);

    Serial.printf("LoRa Sender\n");

    LoRa.setPins(LORA_SS, LORA_RESET, LORA_DIO0);

    LoRa.setSyncWord(0xE2);
    LoRa.onReceive(onReceive);
    LoRa.receive();
    LoRa.onTxDone(onTxDone);
    LoRa.onCrcError(onCrcError);
    LoRa.enableCrc();

    while (!LoRa.begin(433E6)) {
        Serial.print(".");
        delay(500);
    }

    Serial.printf("\nLoRa Initializing OK!\n");

    limited_imbuff = (uint8_t *)malloc(LIMITED_IMBUFF_MAX_SIZE);
    assert(limited_imbuff);
    Serial.printf("Alloc ok!\n");

    CustomPort.begin(115200, SERIAL_8N1, 16, 17); ///RX, TX.
    CustomPort.setRxBufferSize(65536);
    
    if(!CustomPort) {
        Serial.printf("CustomPort failed to start..\n");
    } else {
        Serial.printf("CustomPort started!\n");
    }

    bool canStart = false;  
    int consecutiveStartBytes = 0;

    uint8_t ch;
    while (!canStart) {
        while (CustomPort.available() > 0 && consecutiveStartBytes < CONSECUTIVE_NEEDED_START_BYTES) {
            ch = CustomPort.read();
            if (ch == START_BYTE) {
                consecutiveStartBytes++;
            } else {
                consecutiveStartBytes = 0;
            }
        }

        if (consecutiveStartBytes >= CONSECUTIVE_NEEDED_START_BYTES) {
            canStart = true;
        }
    }

    ///nu mai dau START_PICS aici, doar pasez bytes-ii la lora.
    Serial.printf("Will start receiving images.\n");
}

void loop() {
    uint8_t ch;
    if (state == STATE_IDLE) {
        CustomPort.write(0); ///doar scriu un caracter.
        
        limitedImbuffSize = 0;
        limitedImbuffIndex = 0;
        limitedImbuffPrevIndex = 0;
        
        remainingImageBytes = 0;
        receivedSizeBytes = 0;
        state = STATE_GET_SIZE;
    } else if (state == STATE_GET_SIZE) {
        while (CustomPort.available() > 0 && receivedSizeBytes < 4) {
            ch = CustomPort.read();
            limited_imbuff[limitedImbuffSize] = ch;
            limitedImbuffSize++;

            receivedSizeBytes++;
            remainingImageBytes = (remainingImageBytes << 8) | ch;
        }

        if (receivedSizeBytes >= 4) {
            Serial.printf("remainingImageBytes = %d\n", remainingImageBytes);
            state = STATE_GET_BYTES;
        }
    } else if (state == STATE_GET_BYTES) {
        while (CustomPort.available() > 0 && remainingImageBytes > 0) {
            ch = CustomPort.read();
            if (limitedImbuffSize < LIMITED_IMBUFF_MAX_SIZE) {
                limited_imbuff[limitedImbuffSize] = ch;
                limitedImbuffSize++;
            }

            remainingImageBytes--;
        }

        if (remainingImageBytes <= 0) {
            ///daca am depasit LIMITED_IMBUFF_MAX_SIZE, modific si remainingImageBytes ca sa pot sa trec ulterior si la alta imagine.
            if (limitedImbuffSize >= LIMITED_IMBUFF_MAX_SIZE) {
                limited_imbuff[0] = 0;
                limited_imbuff[1] = 0;
                limited_imbuff[2] = 0xFF;
                limited_imbuff[3] = 0xFB;
            }

            state = STATE_SEND_LORA;
        }
    } else if (state == STATE_SEND_LORA) {
        if (limitedImbuffIndex < limitedImbuffSize) {
            state = STATE_WAIT_TX;
            justReceivedAck = false;
            LoRa.beginPacket();
            limitedImbuffPrevIndex = limitedImbuffIndex;
            for (int i = 0; limitedImbuffIndex < limitedImbuffSize && i < LORA_PACKET_SIZE; limitedImbuffIndex++, i++) {
                LoRa.write(limited_imbuff[limitedImbuffIndex]);
            }
            LoRa.endPacket();

            Serial.printf("ind %d size %d\n", limitedImbuffIndex, limitedImbuffSize);
        } else {
            state = STATE_IDLE;
        }
    } else if (state == STATE_WAIT_TX) {
        ///astept callback onTxDone.
    } else if (state == STATE_WAIT_ACK) {
        if (justReceivedAck) {
            justReceivedAck = false;
            state = STATE_SEND_LORA;

            if (isCrcOk) {
                ch = LoRa.read();
                while (LoRa.available()) {
                    LoRa.read();
                }

                if (ch == 0) {
                    Serial.printf("ack succes.\n");
                    ///ack succes. pot sa trec la urmatoarea parte.
                } else {
                    Serial.printf("ack fail 1.\n");
                    limitedImbuffIndex = limitedImbuffPrevIndex; ///ack fail. trimit din nou ultima parte.
                }
            } else {
                Serial.printf("ack fail 2.\n");
                limitedImbuffIndex = limitedImbuffPrevIndex; ///nici macar ack-ul nu este consistent. trimit din nou ultima parte.
            }
        }
    }
}
