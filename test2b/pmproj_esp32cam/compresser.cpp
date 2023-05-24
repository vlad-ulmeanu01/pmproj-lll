#include "compresser.h"

namespace compresser {
    int getChild(int r, int id) {
        return ((r / 3) * 4 + id) * 3;
    }

    int getFather(int r) {
        return ((r / 3) - 1) / 4 * 3;
    }

    int whichChild(int r) {
        return (r - getChild(getFather(r), 1)) / 3 + 1;
    }

    uint16_t getSplitActions(int r, uint8_t *cnt_splits) {
        uint16_t split_actions = 0;
        *cnt_splits = 0;

        int j;
        while (r > 0) {
            (*cnt_splits)++;
            split_actions <<= 2;

            j = whichChild(r);
            if (j == 3 || j == 4) split_actions |= 2;
            if (j == 2 || j == 4) split_actions |= 1;

            r = getFather(r);
        }

        return split_actions;
    }

    uint8_t getAmountOfCommonSplits(uint8_t cnt_splits1, uint16_t split_actions1,
                                    uint8_t cnt_splits2, uint16_t split_actions2) {
        uint8_t cnt = (cnt_splits1 < cnt_splits2? cnt_splits1: cnt_splits2), ans = 0;
        while (ans < cnt && (split_actions1 & 3) == (split_actions2 & 3)) {
            ans++;
            split_actions1 >>= 2;
            split_actions2 >>= 2;
        }

        return ans;
    }

    void lazy_reset(CompressedImage *cim) {
        cim->len = 0;
        cim->occupied_bits = 0;
        cim->buff[0] = 0;
    }

    void add_bits(CompressedImage *cim, uint16_t x, int amt) {
        for (amt--; amt >= 0; amt--) {
            cim->buff[cim->len] |= (((x & (1<<amt)) >> amt) << (7 - cim->occupied_bits));
            cim->occupied_bits++;
            if (cim->occupied_bits == 8) {
                cim->len++;
                cim->occupied_bits = 0;
                cim->buff[cim->len] = 0;
            }
        }
    }
  
    void
    compress(
        CompressedImage *cim,
        uint8_t *im,
        uint8_t *quad_help_min,
        uint8_t *quad_help_max)
    {
        int i, j, x, y, z, chan;

        ///pentru un pixel compresat:
        uint8_t cnt_splits; ///1 split = un nivel in jos in arborele quad. .. \in [1, 8].
        ///!!!oblig sa fac minim un split ca sa salvez un bit la cnt_splits.

        uint16_t split_actions; ///daca un split m-a dus in coltul dreapta sus, adaug "01" in split_actions.
        ///(st sus, dr sus, st jos, dr jos) = (00, 01, 10, 11).
        ///2 splituri, dr sus apoi st sus => ..(00)(01) (... nu conteaza).

        uint16_t rgb565; ///ce culoare are pixelul.

        ///parcurgere construire quad tree (spate -> fata).

        ///pun valorile pentru pixeli (frunzele din quad tree).
        ///ultimul nivel este organizat in forma de fractal (Z). incerc sa imi dau seama pentru fiecare pozitie
        ///de pe ultimul nivel ce pixel ii corespunde.
        for (i = 0, z = QUAD_TREE_SIZE-3; i < BUFF888_SIZE; i += 3, z -= 3) {
            split_actions = getSplitActions(z, &cnt_splits);

            x = y = 0;
            for (int _ = 0, p2 = (IMG_LEN >> 1); _ < 8; _++, p2 >>= 1) {
                switch (split_actions & 3) {
                    case 1: x += p2; break;
                    case 2: y += p2; break;
                    case 3: x += p2; y += p2; break;
                    default: break; ///pentru 0 nu fac nimic.
                };

                split_actions >>= 2;
            }

            j = 3 * (y * IMG_LEN + x);
            for (chan = 0; chan < 3; chan++) {
                quad_help_min[z+chan] = quad_help_max[z+chan] = im[j+chan];
            }
        }

        ///fac restul nivelelor din quad tree.

        int fiu;
        for (; z >= 0; z -= 3) {
            for (chan = 0; chan < 3; chan++) {
                ///fiu = unde incepe canalul r.
                for (i = 1, fiu = getChild(z, 1); i <= 4; i++, fiu += 3) {
                    if (i == 1 || quad_help_min[z+chan] > quad_help_min[fiu+chan]) {
                        quad_help_min[z+chan] = quad_help_min[fiu+chan];
                    }

                    if (i == 1 || quad_help_max[z+chan] < quad_help_max[fiu+chan]) {
                        quad_help_max[z+chan] = quad_help_max[fiu+chan];
                    }
                }
            }
        }

        uint8_t Min, Max;
        uint16_t r, g, b;
        int log4arie = 7, p4 = 4, arie_lim = 4; ///nu pornesc fix din radacina.

        uint8_t cnt_common_splits = 0, prev_cnt_splits = 0;
        uint16_t prev_split_actions = 0; ///de obicei, spliturile la 2 pixeli consecutivi sunt aproape la fel.
        
        lazy_reset(cim);

        ///din conventie, trebuie sa dau split minim o data la imagine.
        for (z = 3; z < QUAD_TREE_SIZE; z += 3) {
            i = getFather(z);

            ///update arie curenta.
            if (z > arie_lim * 3) { ///z/3 a depasit arie_lim.
                log4arie--;
                p4 <<= 2;
                arie_lim += p4;
            }

            ///daca nu este adevarata conditia, deja l-am compresat pe tata, deci nu mai continui cautarea in subarborele lui.
            if (quad_help_max[i] >= quad_help_min[i]) {
                ///pot sa compresez pixelul acesta sau dau split?
                for (Min = 255, Max = 0, chan = 0; chan < 3; chan++) {
                    Min = (Min > quad_help_min[z + chan]? quad_help_min[z + chan]: Min);
                    Max = (Max < quad_help_max[z + chan]? quad_help_max[z + chan]: Max);
                }

                ///TODO sa transmit pixeli? se merita?
                if (log4arie == 0 || util::coef_corector_lookup[log4arie] * (Max - Min) <= DISP_THRESH) {
                    ///compresez.

                    r = ((uint16_t)quad_help_min[z] + quad_help_max[z]) >> 1;
                    g = ((uint16_t)quad_help_min[z+1] + quad_help_max[z+1]) >> 1;
                    b = ((uint16_t)quad_help_min[z+2] + quad_help_max[z+2]) >> 1;

                    rgb565 = 0;
                    rgb565 |= (r & 0b11111000) << 8;
                    rgb565 |= (g & 0b11111100) << 3;
                    rgb565 |= (b & 0b11111000) >> 3;

                    ///calculez numarul de splituri si modul in care le-am facut.
                    split_actions = getSplitActions(z, &cnt_splits);

                    ///marchez nodul ca fiind compresat ca sa nu mai explorez fiii mai tarziu.
                    ///(conditie imposibila ca sa nu mai am nevoie de alti vectori).
                    quad_help_min[z] = 255;
                    quad_help_max[z] = 0;

                    if (cim->len == 0 && cim->occupied_bits == 0) {
                        cnt_common_splits = 0; ///e primul pixel compresat pe care il pun.
                    } else {
                        cnt_common_splits = getAmountOfCommonSplits(prev_cnt_splits, prev_split_actions,
                                                                    cnt_splits, split_actions);
                    }

                    prev_cnt_splits = cnt_splits;
                    prev_split_actions = split_actions;

                    split_actions >>= (2 * cnt_common_splits); ///scot prefixul comun.

                    ///trec in buffer-ul de output pixelul compresat.
                    add_bits(cim, cnt_splits, 3); ///conventie. daca cnt_splits == 8, transmit 0.
                    add_bits(cim, cnt_common_splits, 3); ///nu are cum sa fie 8 ptc ar insemna ca am acelasi pixel compresat.
                    add_bits(cim, split_actions, 2 * (cnt_splits - cnt_common_splits));
                    add_bits(cim, rgb565, 16);
                }
            } else {
                ///trebuie sa marchez si nodul asta! altfel decizia tatalul nu se mai propaga in jos.
                quad_help_min[z] = 255;
                quad_help_max[z] = 0;
            }
        }
    }
}
