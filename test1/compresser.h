#ifndef __COMPRESSER_H__
#define __COMPRESSER_H__

#include "util.h"

namespace compresser {
    struct CompressedImage {
        int len; ///lungimea bufferului cu imaginea compresata.
        int occupied_bits; ///cati biti sunt ocupati in byte-ul curent.
        uint8_t *buff; ///unde incepe bufferul. !! buff este alocat pe heap.
        int entries; ///(debug) cati pixeli compresati am.
    };

    /**
     * reseteaza structura imaginii compresate.
     */
    void lazy_reset(CompressedImage *cim);

    /**
     * adaug la imaginea compresata cei mai mici amt biti din x.
     */
    void add_bits(CompressedImage *cim, uint16_t x, int amt);

    /**
     * Sunt intr-un pixel rosu. Care este al id-lea meu fiu. id \in {1, 2, 3, 4}.
     */
    int getChild(int r, int id);

    /**
     * Cine este tatal lui r?
     */
    int getFather(int r);

    /**
     * Al catelea copil este r? ans \in {1, 2, 3, 4}.
     */
    int whichChild(int r);

    /**
     * ce se intoarce:
     * daca un split m-a dus in coltul dreapta sus, adaug "01" in split_actions.
     * (st sus, dr sus, st jos, dr jos) = (00, 01, 10, 11).
     * 2 splituri, dr sus apoi st sus => ..(00)(01) (... nu conteaza).
     * 
     * r = istoricul carui nod vreau sa-l aflu. trebuie 3 | r neaparat.
     * *cnt_splits = cate splituri am facut in total.
     */
    uint16_t getSplitActions(int r, uint8_t *cnt_splits);

    /**
     * intoarce imaginea in format compresat.
     * 
     * cim: pointer catre bufferul unde se intoarce imaginea compresata.
     * 
     * cpx_help: pointer catre bufferul ajutator in care fac calcule partiale. Ar tb sa aiba IMG_LEN^2 pozitii.
     * 
     * im: Un buffer cu o imagine 256x256 in format RGB888.
     * 
     * quad_help_min/max: Se garanteaza ca quad_help este un buffer cu 4^9 - 1 pozitii.
     * (pt fiecare culoare am nevoie de 4^0+4^1+..+4^8 pozitii).
     * 
     * quad_help este stocat in formatul rgbrgbrgb...
     * am doua buffere, unul pentru valori minime si unul pentru valori maxime din careuri.
     * daca un pixel red este stocat la pozitia r, cei 4 fii ai lui sunt la pozitiile ((r/3)*4 + 1/2/3/4) * 3.
     * tata(r) = ((x/3 - 1) / 4) * 3.
     */
    void compress(CompressedImage *cim,
                  uint8_t *im,
                  uint8_t *quad_help_min,
                  uint8_t *quad_help_max);
}

#endif
