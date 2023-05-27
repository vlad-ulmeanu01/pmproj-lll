///sender.

#include <SPI.h>
#include <LoRa.h>

#define LORA_PACKET_SIZE 220
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

#define ANSWER_TIMEOUT 2000 ///daca nu primesc un raspuns la pachetul trimis dupa 2s, retrimit pachetul.

#define STATE_SENDING 0 ///trimit un pachet.
#define STATE_WAIT_TXDONE 1 ///astept sa finalizez trimiterea pachetului.
#define STATE_WAIT_ACK 2 ///astept ack.

uint8_t starting_ch = 0;

int prevPack[LORA_PACKET_SIZE];
volatile int state = STATE_SENDING, packetSize = 0;
volatile bool justReceivedAck = false, isCrcOk = true;
volatile unsigned long lastSendTime = 0;

void onReceive(int psz) {
    packetSize = psz;
    justReceivedAck = true;
    isCrcOk = true;
}

void onTxDone() {
    //Serial.printf("onTxDone begin.\n");

    state = STATE_WAIT_ACK;
    lastSendTime = millis();
    LoRa.receive();
}

void onCrcError() {
    //Serial.printf("onCrcError!!\n");
    justReceivedAck = true;
    isCrcOk = false;
}

void setup() {
    Serial.begin(115200);
    Serial.printf("LoRa sender.\n");

    LoRa.setPins(LORA_SS, LORA_RESET, LORA_DIO0);

    while (!LoRa.begin(433E6)) {
        Serial.printf(".");
        delay(500);
    }

    LoRa.setSyncWord(0xE2);
    LoRa.onReceive(onReceive);
    LoRa.receive();
    LoRa.onTxDone(onTxDone);
    LoRa.onCrcError(onCrcError);
    LoRa.enableCrc();

    Serial.printf("\nLoRa init finished.\n");
}

void loop() {
    //Serial.printf("state = %d\n", state);
    if (state == STATE_SENDING) {
        state = STATE_WAIT_TXDONE;
        LoRa.beginPacket();
        uint8_t ch = starting_ch;
        for (int i = 0; i < LORA_PACKET_SIZE; i++, ch++) {
            LoRa.write(ch);
            prevPack[i] = ch;
        }
        LoRa.endPacket();
    } else if (state == STATE_WAIT_TXDONE) {
        ///astept sa imi iau interrupt.
    } else if (state == STATE_WAIT_ACK) {
        ///daca astept ack sigur am trimis un pachet.

        if (millis() - lastSendTime > ANSWER_TIMEOUT) {
            ///trimit din nou.
            Serial.printf("Resending packet.\n");

            state = STATE_WAIT_TXDONE;
            LoRa.beginPacket();
            for (int i = 0; i < LORA_PACKET_SIZE; i++) {
                LoRa.write(prevPack[i]);
            }
            LoRa.endPacket();
        } else if (justReceivedAck) {
            justReceivedAck = false;
            state = STATE_SENDING;

            if (isCrcOk) {
                int cnt0 = 0, cnt1 = 0;
                uint8_t ch = LoRa.read();

                while (LoRa.available()) {
                    LoRa.read();
                }
    
                if (ch == 0) {
                    Serial.printf("Tocmai am primit ack pt pachet primit corect. (starting_ch = %d).\n", starting_ch);
                    starting_ch++;  
                } else {
                    Serial.printf("Trebuie sa trimit din nou pachetul cu starting_ch = %d...\n", starting_ch);
                }
            } else {
                Serial.printf("(crc bad) Trebuie sa trimit din nou pachetul cu starting_ch = %d...\n", starting_ch);
            }
        }
    }
}
