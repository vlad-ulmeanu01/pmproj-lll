///receiver.

#include <SPI.h>
#include <LoRa.h>

#define START_BYTE 0xE2
#define CONSECUTIVE_NEEDED_START_BYTES 32
#define START_PICS(_b) for (int _ = 0; _ < CONSECUTIVE_NEEDED_START_BYTES; _++) Serial.write(_b);

#define LORA_PACKET_SIZE 240
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

int pack[LORA_PACKET_SIZE];
volatile bool justReceivedPack = false;
volatile int packetSize = 0;

void onReceive(int psz) {
    packetSize = psz;
    for (int i = 0; i < psz; i++) {
        pack[i] = LoRa.read();
    }

    justReceivedPack = true;
}

void setup() {
    Serial.begin(115200);

    LoRa.setPins(LORA_SS, LORA_RESET, LORA_DIO0);

    while (!LoRa.begin(433E6)) {
        Serial.printf(".");
        delay(500);
    }

    Serial.printf("\n");

    LoRa.setSyncWord(0xE2);
    LoRa.onReceive(onReceive); ///register the receive callback.
    LoRa.receive(); ///put the radio into receive mode.
    
    Serial.printf("LoRa Initializing OK!\n");

    START_PICS(START_BYTE)
}

void loop() {
    if (justReceivedPack) {
        //Serial.printf("packetSize = %d. (%d, %d, %d, %d)\n", packetSize, pack[0], pack[1], pack[2], pack[3]);
        for (int i = 0; i < packetSize; i++) {
            Serial.write(pack[i]);
        }

        justReceivedPack = false;
    }
}
