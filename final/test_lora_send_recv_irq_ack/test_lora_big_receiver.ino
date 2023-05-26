///receiver.

#include <SPI.h>
#include <LoRa.h>

#define LORA_PACKET_SIZE 220
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

int pack[LORA_PACKET_SIZE];
volatile bool justReceivedPack = false, isCrcOk = true, isTxComplete = true;
volatile int packetSize = 0;

void onReceive(int psz) {
    //Serial.printf("onReceive begin.\n");
    packetSize = psz;
    justReceivedPack = true;
    isCrcOk = true;
}

void onTxDone() {
    //Serial.printf("onTxDone begin.\n");
    isTxComplete = true;
}

void onCrcError() {
    //Serial.printf("onCrcError begin.\n");
    justReceivedPack = true;
    isCrcOk = false;
}

void setup() {
    Serial.begin(115200);

    LoRa.setPins(LORA_SS, LORA_RESET, LORA_DIO0);

    while (!LoRa.begin(433E6)) {
        Serial.printf(".");
        delay(500);
    }

    LoRa.setSyncWord(0xE2);
    LoRa.onReceive(onReceive); ///receive callback. daca primesc un pachet cu crc ok.
    LoRa.receive();
    LoRa.onTxDone(onTxDone); ///apelat cand am terminat de transmis.
    LoRa.onCrcError(onCrcError); ///daca primesc un pachet cu crc busit.
    LoRa.enableCrc();

    Serial.printf("\nLoRa Initializing OK!\n");
}

void loop() {
    if (!isTxComplete) {
        return;
    }
    
    if (justReceivedPack) {
        justReceivedPack = false;

        if (isCrcOk) {
            for (int i = 0; i < packetSize; i++) {
                pack[i] = LoRa.read();
            }

            Serial.printf("primit ok! pack[0] = %d\n", pack[0]);

            isTxComplete = false;
            LoRa.beginPacket();
            for (int _ = 0; _ < 8; _++) LoRa.write(0);
            LoRa.endPacket();
            LoRa.receive();
        } else {
            while (LoRa.available()) {
                LoRa.read();
            }

            Serial.printf("primit bushit.\n");

            isTxComplete = false;
            LoRa.beginPacket();
            for (int _ = 0; _ < 8; _++) LoRa.write(1);
            LoRa.endPacket();
            LoRa.receive();
        }
    }
}
