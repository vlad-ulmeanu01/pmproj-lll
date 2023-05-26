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
#define STATE_GET_BYTES 2 ///inca astept sa primesc toti bytes-ii din imagine.

#define LORA_PACKET_SIZE 200 ///payload-ul are maxim 240 bytes per pachet.
#define LORA_SS 5
#define LORA_RESET 14
#define LORA_DIO0 2

uint8_t ch;
int state = 0;
int remainingImageBytes = 0, receivedSizeBytes = 0;

int lora_buffer_counter = 0;

//volatile bool lora_ack = false;
//void onTxDone() {
//    lora_ack = true;
//}

void lora_wait_ack() {
//    delay(10);
//    lora_ack = false;
//    while (!lora_ack) {
//        delay(5);
//    }

    int packetSize = LoRa.parsePacket();
    unsigned long now;
    while (packetSize == 0 && millis() - now < 1000) {
        packetSize = LoRa.parsePacket();
    }
    
    while (LoRa.available() > 0) {
        LoRa.read();
    }
}

void lora_add_byte(uint8_t x) {
    if (lora_buffer_counter == 0) {
        LoRa.beginPacket();
    }

    LoRa.write(x);
    lora_buffer_counter++;
    Serial.printf("(lora_add_byte) lora_buffer_counter = %d\n", lora_buffer_counter);

    if (lora_buffer_counter >= LORA_PACKET_SIZE) {
        LoRa.endPacket();
        Serial.printf("(lora_add_byte) Sent packet.\n");
        
        lora_wait_ack();
        Serial.printf("(lora_add_byte) got ack.\n");
        
        lora_buffer_counter = 0;
    }
}

void lora_force_flush() {
    if (lora_buffer_counter > 0) {
        LoRa.endPacket();
        Serial.printf("(lora_force_flush) Sent packet.\n");

        lora_wait_ack();
        Serial.printf("(lora_force_flush) got ack.\n");
    }
}

void setup() {
    Serial.begin(115200);

    Serial.println("LoRa Sender");

    LoRa.setPins(LORA_SS, LORA_RESET, LORA_DIO0);
    //LoRa.onTxDone(onTxDone); ///!!!tb onTxDone pe sender.

    while (!LoRa.begin(433E6)) {
        Serial.print(".");
        delay(500);
    }

    LoRa.setSyncWord(0xE2);

    Serial.printf("LoRa Initializing OK!\n");

    CustomPort.begin(115200, SERIAL_8N1, 16, 17); ///RX, TX.
    CustomPort.setRxBufferSize(65536);
    
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
        }

        if (consecutiveStartBytes >= CONSECUTIVE_NEEDED_START_BYTES) {
            canStart = true;
        }
    }

    ///nu mai dau START_PICS aici, doar pasez bytes-ii la lora.
    Serial.printf("Will start receiving images.\n");
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
            lora_add_byte(ch);
            receivedSizeBytes++;
            remainingImageBytes = (remainingImageBytes << 8) | ch;
        }

        if (receivedSizeBytes >= 4) {
            state = STATE_GET_BYTES;
        }
    } else { ///STATE_GET_BYTES.
        while (CustomPort.available() > 0 && remainingImageBytes > 0) {
            ch = CustomPort.read();
            lora_add_byte(ch);
            remainingImageBytes--;
        }

        if (remainingImageBytes <= 0) {
            lora_force_flush();
            state = STATE_IDLE;
        }
    }
}
