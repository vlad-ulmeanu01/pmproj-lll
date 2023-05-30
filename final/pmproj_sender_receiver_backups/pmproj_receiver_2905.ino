///Receiver, ESP32-WROOMU.

#include <SPI.h>
#include <LoRa.h>

#define START_BYTE 0xE2
#define CONSECUTIVE_NEEDED_START_BYTES 32
#define START_PICS(_b) for (int _ = 0; _ < CONSECUTIVE_NEEDED_START_BYTES; _++) Serial.write(_b);

#define LORA_PACKET_SIZE 220
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

#define ANSWER_TIMEOUT 2000
#define PACK_BUFFER_MAX_SIZE 65536

uint8_t pack[LORA_PACKET_SIZE], prevPack[LORA_PACKET_SIZE];
uint8_t *pack_buffer = NULL; ///tb sa trimit ack imd cu primesc pachet. nu pot sa astept dupa seriala.
int pack_buffer_ind = 0;

volatile bool justReceivedPack = false, isCrcOk = true, isTxComplete = true;
volatile int packetSize = 0, prevPacketSize = -1;
volatile unsigned long lastSendTime = 0;
volatile uint8_t lastAckValue = 0;

void onReceive(int psz) {
    packetSize = psz;
    justReceivedPack = true;
    isCrcOk = true;
}

void onTxDone() {
    isTxComplete = true;
    lastSendTime = millis();
    LoRa.receive();
}

void onCrcError() {
    justReceivedPack = true;
    isCrcOk = false;
}

void setup() {
    Serial.begin(115200);

    pack_buffer = (uint8_t *)malloc(PACK_BUFFER_MAX_SIZE);
    assert(pack_buffer);

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
            //Serial.printf("got pack! packetSize = %d\n", packetSize);

            for (int i = 0; i < packetSize; i++) {
                pack[i] = LoRa.read();
            }

            ///raspund cu ack afirmativ <=> astept urmatorul pachet.
            isTxComplete = false;
            lastAckValue = 0;
            LoRa.beginPacket();
            LoRa.write(0);
            LoRa.endPacket();

            if (packetSize == prevPacketSize && memcmp(pack, prevPack, packetSize) == 0) {
                ///pachet duplicat, dau drop.
            } else {
                prevPacketSize = packetSize;
                memcpy(prevPack, pack, packetSize);

                for (int i = 0; i < packetSize; i++) {
                    pack_buffer[pack_buffer_ind] = pack[i];
                    pack_buffer_ind++;
                }
            }
        } else {
            //Serial.printf("got pack with bad ack.\n");
            while (LoRa.available()) {
                LoRa.read();
            }

            ///raspund cu ack negativ <=> astept pachetul curent din nou.
            isTxComplete = false;
            lastAckValue = 1;
            LoRa.beginPacket();
            LoRa.write(1);
            LoRa.endPacket();
        }
    } else if (prevPacketSize != -1 && millis() - lastSendTime > ANSWER_TIMEOUT) {
        //Serial.printf("Resending pack. (lastAckValue = %d)\n", lastAckValue);

        isTxComplete = false;
        LoRa.beginPacket();
        LoRa.write(lastAckValue);
        LoRa.endPacket();
    } else if (pack_buffer_ind > 0) {
        ///nu am absolut nimic altceva de facut in afara de golirea bufferului serial.

        ///TODO daca tot nu merge, opreste forul daca s-a ridicat justReceivedPack si trateaza pack_buffer ca un deque.
        for (int i = 0; i < pack_buffer_ind; i++) {
            Serial.write(pack_buffer[i]);
        }

        pack_buffer_ind = 0;
    }
}
