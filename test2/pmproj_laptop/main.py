import pygame
import serial
import time

from collections import deque

class Buffer:
    def __init__(self):
        self.buff = deque()
        self.now = 7 #care este cel mai mare bit necitit din buff.front().

    def getBit(self):
        if len(self.buff) == 0:
            return None

        ans = ((self.buff[0] & (1 << self.now)) >> self.now)
        self.now -= 1
        if self.now < 0:
            self.buff.popleft()
            self.now = 7
        return ans
    
    def getBits(self, amt):
        ans = 0
        for _ in range(amt):
            ans <<= 1
            bit = self.getBit()
            if bit is None:
                return None
            ans |= bit
        return ans

    def getCompressedPixel(self):
        cntSplits = self.getBits(3)
        if cntSplits is None:
            return None

        if cntSplits == 0:
            cntSplits = 8

        splitActions = self.getBits(2 * cntSplits)
        if splitActions is None:
            return None
        
        x, y, l = 0, 0, 256 #unde incepe patratul compresat, ce marime are.
        for _ in range(cntSplits):
            op = splitActions & 3 #iau ultimii 2 biti.
            splitActions >>= 2
            
            l >>= 1
            if op == 0b00: #stanga sus.
                pass #x, y raman la fel.
            elif op == 0b01:
                x += l
            elif op == 0b10:
                y += l
            else:
                x += l
                y += l
                
        rgb565 = self.getBits(16)
        if rgb565 is None:
            return None

        r = (rgb565 & 0b1111100000000000) >> 8
        g = (rgb565 & 0b0000011111100000) >> 3
        b = (rgb565 & 0b0000000000011111) << 3
        
        return (x, y, l, r, g, b)
        

conn = serial.Serial(port = "COM4",
                     baudrate = 115200,
                     timeout = 0.1,
                     parity = serial.PARITY_NONE,
                     stopbits = serial.STOPBITS_ONE,
                     bytesize = serial.EIGHTBITS)

time.sleep(1)
print("conn started.")

pygame.init()
window = pygame.display.set_mode((256, 256))

#stiu ca primesc pixeli doar dupa 32 de bytes consecutivi pe 0.
startByte = 0xE2
wantedConsecutiveBytes = 32
canRecvPixels = False
consecutiveBytes = 0

remainingImageBytes = None

buffer = Buffer()
while True:
    #citesc toti bytes-ii posibili.
    bytes = conn.read_all()

    #vad daca pot primi pixeli ce tin de imagini. dupa ce am confirmare, pun bytes-ii in buffer.
    if len(bytes):
        for b in bytes:
            b = int(b)
            #print(f"Citit b = {b}")

            if not canRecvPixels:
                if b != startByte:
                    consecutiveBytes = 0
                else:
                    consecutiveBytes += 1
                    if consecutiveBytes >= wantedConsecutiveBytes:
                        canRecvPixels = True
                        print("Setat canRecvPixels pe True!")
            else:
                buffer.buff.append(b)
    
    #daca da, incerc sa iau intai numarul de bytes ai imaginii.
    if canRecvPixels:
        if remainingImageBytes is None:
            if len(buffer.buff) >= 4:
                remainingImageBytes = 0
                for _ in range(4):
                    remainingImageBytes <<= 8
                    remainingImageBytes |= buffer.buff[0]
                    buffer.buff.popleft()
        elif len(buffer.buff) >= remainingImageBytes: #astept pana se umple tot bufferul cu imaginea curenta.
            #il golesc pe tot deodata.
            canGetPixel = True
            while canGetPixel:
                px = buffer.getCompressedPixel()
                if px is None:
                    canGetPixel = False
                else:
                    x, y, l, r, g, b = px
                    pygame.draw.rect(window, (r, g, b), pygame.Rect(x, y, l, l))
            
            while len(buffer.buff): #scot si ultima portiune de byte ramasa.
                buffer.buff.popleft()
            remainingImageBytes = None #resetez remainingImageBytes ca sa trec la urmatoarea imagine.

    pygame.display.update()
