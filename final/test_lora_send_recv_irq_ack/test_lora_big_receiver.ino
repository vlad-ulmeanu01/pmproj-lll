///receiver.

#include <SPI.h>
#include <LoRa.h>

#define LORA_PACKET_SIZE 220
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

#define ANSWER_TIMEOUT 2000 ///daca nu primesc raspuns la mesaj dupa 2s, retrimit mesajul.

int pack[LORA_PACKET_SIZE], prevPack[LORA_PACKET_SIZE];
volatile bool justReceivedPack = false, isCrcOk = true, isTxComplete = true;
volatile int packetSize = 0, prevPacketSize = -1;
volatile unsigned long lastSendTime = 0;

int lastAckValue = 0;

void onReceive(int psz) {
    //Serial.printf("onReceive begin.\n");
    packetSize = psz;
    justReceivedPack = true;
    isCrcOk = true;
}

void onTxDone() {
    //Serial.printf("onTxDone begin.\n");
    isTxComplete = true;
    lastSendTime = millis();
    LoRa.receive();
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

    if (prevPacketSize != -1 && millis() - lastSendTime > ANSWER_TIMEOUT) {
        Serial.printf("Resending ack (lastAckValue = %d).\n", lastAckValue);
        isTxComplete = false;
        LoRa.beginPacket();
        LoRa.write(lastAckValue);
        LoRa.endPacket();
    } else if (justReceivedPack) {
        justReceivedPack = false;

        if (isCrcOk) {
            for (int i = 0; i < packetSize; i++) {
                pack[i] = LoRa.read();
            }

            if (packetSize == prevPacketSize && memcmp(pack, prevPack, packetSize) == 0) {
                Serial.printf("primit ok, pachet duplicat, drop. pack[0] = %d\n", pack[0]);
            } else {
                Serial.printf("primit ok! pack[0] = %d\n", pack[0]);
                prevPacketSize = packetSize;
                memcpy(prevPack, pack, packetSize);
            }

            isTxComplete = false;
            lastAckValue = 0;
            LoRa.beginPacket();
            LoRa.write(0);
            LoRa.endPacket();
        } else {
            int bytes_available = 0;
            while (LoRa.available()) {
                LoRa.read();
                bytes_available++;
            }

            Serial.printf("primit bushit (%d bytes disponibili in buffer).\n", bytes_available);

            isTxComplete = false;
            lastAckValue = 1;
            LoRa.beginPacket();
            LoRa.write(1);
            LoRa.endPacket();
        }
    }
}
