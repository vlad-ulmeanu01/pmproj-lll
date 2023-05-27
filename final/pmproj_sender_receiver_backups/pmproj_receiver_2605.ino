///receiver.

#include <SPI.h>
#include <LoRa.h>

#define START_BYTE 0xE2
#define CONSECUTIVE_NEEDED_START_BYTES 32
#define START_PICS(_b) for (int _ = 0; _ < CONSECUTIVE_NEEDED_START_BYTES; _++) Serial.write(_b);

#define LORA_PACKET_SIZE 220
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

uint8_t pack[LORA_PACKET_SIZE];
volatile bool justReceivedPack = false, isCrcOk = true, isTxComplete = true;
volatile int packetSize = 0;

void onReceive(int psz) {
    packetSize = psz;
    justReceivedPack = true;
    isCrcOk = true;
}

void onTxDone() {
    isTxComplete = true;
    LoRa.receive();
}

void onCrcError() {
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
    LoRa.receive(); ///trec pe modul listening. ies automat cand dau beginPacket. trebuie sa intru la loc dupa endPacket.
    LoRa.onTxDone(onTxDone); ///apelat cand am terminat de transmis.
    LoRa.onCrcError(onCrcError); ///daca primesc un pachet cu crc busit.
    LoRa.enableCrc();

    Serial.printf("\nLoRa Initializing OK!\n");

    START_PICS(START_BYTE)
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
    
            for (int i = 0; i < packetSize; i++) {
                Serial.write(pack[i]);
            }

            ///raspund cu ack afirmativ <=> astept urmatorul pachet.
            isTxComplete = false;
            LoRa.beginPacket();
            LoRa.write(0);
            LoRa.endPacket();
        } else {
            while (LoRa.available()) {
                LoRa.read();
            }

            ///raspund cu ack negativ <=> astept pachetul curent din nou.
            isTxComplete = false;
            LoRa.beginPacket();
            LoRa.write(1);
            LoRa.endPacket();
        }
    }
}
