/* This file was generated automatically by the Snowball to ISO C compiler */
/* http://snowballstem.org/ */
static int irish_UTF_8_stem(struct SN_env * z);
static int irish_UTF_8_r_verb_sfx(struct SN_env * z);
static int irish_UTF_8_r_deriv(struct SN_env * z);
static int irish_UTF_8_r_noun_sfx(struct SN_env * z);
static int irish_UTF_8_r_mark_regions(struct SN_env * z);
static int irish_UTF_8_r_initial_morph(struct SN_env * z);
static int irish_UTF_8_r_RV(struct SN_env * z);
static int irish_UTF_8_r_R2(struct SN_env * z);
static int irish_UTF_8_r_R1(struct SN_env * z);


static struct SN_env * irish_UTF_8_create_env(struct SN_env *z);
static void irish_UTF_8_close_env(struct SN_env * z);


static const symbol irish_UTF_8_s_0_0[2] = { 'b', '\'' };
static const symbol irish_UTF_8_s_0_1[2] = { 'b', 'h' };
static const symbol irish_UTF_8_s_0_2[3] = { 'b', 'h', 'f' };
static const symbol irish_UTF_8_s_0_3[2] = { 'b', 'p' };
static const symbol irish_UTF_8_s_0_4[2] = { 'c', 'h' };
static const symbol irish_UTF_8_s_0_5[2] = { 'd', '\'' };
static const symbol irish_UTF_8_s_0_6[4] = { 'd', '\'', 'f', 'h' };
static const symbol irish_UTF_8_s_0_7[2] = { 'd', 'h' };
static const symbol irish_UTF_8_s_0_8[2] = { 'd', 't' };
static const symbol irish_UTF_8_s_0_9[2] = { 'f', 'h' };
static const symbol irish_UTF_8_s_0_10[2] = { 'g', 'c' };
static const symbol irish_UTF_8_s_0_11[2] = { 'g', 'h' };
static const symbol irish_UTF_8_s_0_12[2] = { 'h', '-' };
static const symbol irish_UTF_8_s_0_13[2] = { 'm', '\'' };
static const symbol irish_UTF_8_s_0_14[2] = { 'm', 'b' };
static const symbol irish_UTF_8_s_0_15[2] = { 'm', 'h' };
static const symbol irish_UTF_8_s_0_16[2] = { 'n', '-' };
static const symbol irish_UTF_8_s_0_17[2] = { 'n', 'd' };
static const symbol irish_UTF_8_s_0_18[2] = { 'n', 'g' };
static const symbol irish_UTF_8_s_0_19[2] = { 'p', 'h' };
static const symbol irish_UTF_8_s_0_20[2] = { 's', 'h' };
static const symbol irish_UTF_8_s_0_21[2] = { 't', '-' };
static const symbol irish_UTF_8_s_0_22[2] = { 't', 'h' };
static const symbol irish_UTF_8_s_0_23[2] = { 't', 's' };

static const struct among irish_UTF_8_a_0[24] =
{
/*  0 */ { 2, irish_UTF_8_s_0_0, -1, 1, 0},
/*  1 */ { 2, irish_UTF_8_s_0_1, -1, 4, 0},
/*  2 */ { 3, irish_UTF_8_s_0_2, 1, 2, 0},
/*  3 */ { 2, irish_UTF_8_s_0_3, -1, 8, 0},
/*  4 */ { 2, irish_UTF_8_s_0_4, -1, 5, 0},
/*  5 */ { 2, irish_UTF_8_s_0_5, -1, 1, 0},
/*  6 */ { 4, irish_UTF_8_s_0_6, 5, 2, 0},
/*  7 */ { 2, irish_UTF_8_s_0_7, -1, 6, 0},
/*  8 */ { 2, irish_UTF_8_s_0_8, -1, 9, 0},
/*  9 */ { 2, irish_UTF_8_s_0_9, -1, 2, 0},
/* 10 */ { 2, irish_UTF_8_s_0_10, -1, 5, 0},
/* 11 */ { 2, irish_UTF_8_s_0_11, -1, 7, 0},
/* 12 */ { 2, irish_UTF_8_s_0_12, -1, 1, 0},
/* 13 */ { 2, irish_UTF_8_s_0_13, -1, 1, 0},
/* 14 */ { 2, irish_UTF_8_s_0_14, -1, 4, 0},
/* 15 */ { 2, irish_UTF_8_s_0_15, -1, 10, 0},
/* 16 */ { 2, irish_UTF_8_s_0_16, -1, 1, 0},
/* 17 */ { 2, irish_UTF_8_s_0_17, -1, 6, 0},
/* 18 */ { 2, irish_UTF_8_s_0_18, -1, 7, 0},
/* 19 */ { 2, irish_UTF_8_s_0_19, -1, 8, 0},
/* 20 */ { 2, irish_UTF_8_s_0_20, -1, 3, 0},
/* 21 */ { 2, irish_UTF_8_s_0_21, -1, 1, 0},
/* 22 */ { 2, irish_UTF_8_s_0_22, -1, 9, 0},
/* 23 */ { 2, irish_UTF_8_s_0_23, -1, 3, 0}
};

static const symbol irish_UTF_8_s_1_0[7] = { 0xC3, 0xAD, 'o', 'c', 'h', 't', 'a' };
static const symbol irish_UTF_8_s_1_1[8] = { 'a', 0xC3, 0xAD, 'o', 'c', 'h', 't', 'a' };
static const symbol irish_UTF_8_s_1_2[3] = { 'i', 'r', 'e' };
static const symbol irish_UTF_8_s_1_3[4] = { 'a', 'i', 'r', 'e' };
static const symbol irish_UTF_8_s_1_4[3] = { 'a', 'b', 'h' };
static const symbol irish_UTF_8_s_1_5[4] = { 'e', 'a', 'b', 'h' };
static const symbol irish_UTF_8_s_1_6[3] = { 'i', 'b', 'h' };
static const symbol irish_UTF_8_s_1_7[4] = { 'a', 'i', 'b', 'h' };
static const symbol irish_UTF_8_s_1_8[3] = { 'a', 'm', 'h' };
static const symbol irish_UTF_8_s_1_9[4] = { 'e', 'a', 'm', 'h' };
static const symbol irish_UTF_8_s_1_10[3] = { 'i', 'm', 'h' };
static const symbol irish_UTF_8_s_1_11[4] = { 'a', 'i', 'm', 'h' };
static const symbol irish_UTF_8_s_1_12[6] = { 0xC3, 0xAD, 'o', 'c', 'h', 't' };
static const symbol irish_UTF_8_s_1_13[7] = { 'a', 0xC3, 0xAD, 'o', 'c', 'h', 't' };
static const symbol irish_UTF_8_s_1_14[4] = { 'i', 'r', 0xC3, 0xAD };
static const symbol irish_UTF_8_s_1_15[5] = { 'a', 'i', 'r', 0xC3, 0xAD };

static const struct among irish_UTF_8_a_1[16] =
{
/*  0 */ { 7, irish_UTF_8_s_1_0, -1, 1, 0},
/*  1 */ { 8, irish_UTF_8_s_1_1, 0, 1, 0},
/*  2 */ { 3, irish_UTF_8_s_1_2, -1, 2, 0},
/*  3 */ { 4, irish_UTF_8_s_1_3, 2, 2, 0},
/*  4 */ { 3, irish_UTF_8_s_1_4, -1, 1, 0},
/*  5 */ { 4, irish_UTF_8_s_1_5, 4, 1, 0},
/*  6 */ { 3, irish_UTF_8_s_1_6, -1, 1, 0},
/*  7 */ { 4, irish_UTF_8_s_1_7, 6, 1, 0},
/*  8 */ { 3, irish_UTF_8_s_1_8, -1, 1, 0},
/*  9 */ { 4, irish_UTF_8_s_1_9, 8, 1, 0},
/* 10 */ { 3, irish_UTF_8_s_1_10, -1, 1, 0},
/* 11 */ { 4, irish_UTF_8_s_1_11, 10, 1, 0},
/* 12 */ { 6, irish_UTF_8_s_1_12, -1, 1, 0},
/* 13 */ { 7, irish_UTF_8_s_1_13, 12, 1, 0},
/* 14 */ { 4, irish_UTF_8_s_1_14, -1, 2, 0},
/* 15 */ { 5, irish_UTF_8_s_1_15, 14, 2, 0}
};

static const symbol irish_UTF_8_s_2_0[9] = { 0xC3, 0xB3, 'i', 'd', 'e', 'a', 'c', 'h', 'a' };
static const symbol irish_UTF_8_s_2_1[7] = { 'p', 'a', 't', 'a', 'c', 'h', 'a' };
static const symbol irish_UTF_8_s_2_2[5] = { 'a', 'c', 'h', 't', 'a' };
static const symbol irish_UTF_8_s_2_3[8] = { 'a', 'r', 'c', 'a', 'c', 'h', 't', 'a' };
static const symbol irish_UTF_8_s_2_4[6] = { 'e', 'a', 'c', 'h', 't', 'a' };
static const symbol irish_UTF_8_s_2_5[12] = { 'g', 'r', 'a', 'f', 'a', 0xC3, 0xAD, 'o', 'c', 'h', 't', 'a' };
static const symbol irish_UTF_8_s_2_6[5] = { 'p', 'a', 'i', 't', 'e' };
static const symbol irish_UTF_8_s_2_7[3] = { 'a', 'c', 'h' };
static const symbol irish_UTF_8_s_2_8[4] = { 'e', 'a', 'c', 'h' };
static const symbol irish_UTF_8_s_2_9[8] = { 0xC3, 0xB3, 'i', 'd', 'e', 'a', 'c', 'h' };
static const symbol irish_UTF_8_s_2_10[7] = { 'g', 'i', 'n', 'e', 'a', 'c', 'h' };
static const symbol irish_UTF_8_s_2_11[6] = { 'p', 'a', 't', 'a', 'c', 'h' };
static const symbol irish_UTF_8_s_2_12[10] = { 'g', 'r', 'a', 'f', 'a', 0xC3, 0xAD, 'o', 'c', 'h' };
static const symbol irish_UTF_8_s_2_13[7] = { 'p', 'a', 't', 'a', 'i', 'g', 'h' };
static const symbol irish_UTF_8_s_2_14[7] = { 0xC3, 0xB3, 'i', 'd', 'i', 'g', 'h' };
static const symbol irish_UTF_8_s_2_15[8] = { 'a', 'c', 'h', 't', 0xC3, 0xBA, 'i', 'l' };
static const symbol irish_UTF_8_s_2_16[9] = { 'e', 'a', 'c', 'h', 't', 0xC3, 0xBA, 'i', 'l' };
static const symbol irish_UTF_8_s_2_17[6] = { 'g', 'i', 'n', 'e', 'a', 's' };
static const symbol irish_UTF_8_s_2_18[5] = { 'g', 'i', 'n', 'i', 's' };
static const symbol irish_UTF_8_s_2_19[4] = { 'a', 'c', 'h', 't' };
static const symbol irish_UTF_8_s_2_20[7] = { 'a', 'r', 'c', 'a', 'c', 'h', 't' };
static const symbol irish_UTF_8_s_2_21[5] = { 'e', 'a', 'c', 'h', 't' };
static const symbol irish_UTF_8_s_2_22[11] = { 'g', 'r', 'a', 'f', 'a', 0xC3, 0xAD, 'o', 'c', 'h', 't' };
static const symbol irish_UTF_8_s_2_23[10] = { 'a', 'r', 'c', 'a', 'c', 'h', 't', 'a', 0xC3, 0xAD };
static const symbol irish_UTF_8_s_2_24[14] = { 'g', 'r', 'a', 'f', 'a', 0xC3, 0xAD, 'o', 'c', 'h', 't', 'a', 0xC3, 0xAD };

static const struct among irish_UTF_8_a_2[25] =
{
/*  0 */ { 9, irish_UTF_8_s_2_0, -1, 6, 0},
/*  1 */ { 7, irish_UTF_8_s_2_1, -1, 5, 0},
/*  2 */ { 5, irish_UTF_8_s_2_2, -1, 1, 0},
/*  3 */ { 8, irish_UTF_8_s_2_3, 2, 2, 0},
/*  4 */ { 6, irish_UTF_8_s_2_4, 2, 1, 0},
/*  5 */ { 12, irish_UTF_8_s_2_5, -1, 4, 0},
/*  6 */ { 5, irish_UTF_8_s_2_6, -1, 5, 0},
/*  7 */ { 3, irish_UTF_8_s_2_7, -1, 1, 0},
/*  8 */ { 4, irish_UTF_8_s_2_8, 7, 1, 0},
/*  9 */ { 8, irish_UTF_8_s_2_9, 8, 6, 0},
/* 10 */ { 7, irish_UTF_8_s_2_10, 8, 3, 0},
/* 11 */ { 6, irish_UTF_8_s_2_11, 7, 5, 0},
/* 12 */ { 10, irish_UTF_8_s_2_12, -1, 4, 0},
/* 13 */ { 7, irish_UTF_8_s_2_13, -1, 5, 0},
/* 14 */ { 7, irish_UTF_8_s_2_14, -1, 6, 0},
/* 15 */ { 8, irish_UTF_8_s_2_15, -1, 1, 0},
/* 16 */ { 9, irish_UTF_8_s_2_16, 15, 1, 0},
/* 17 */ { 6, irish_UTF_8_s_2_17, -1, 3, 0},
/* 18 */ { 5, irish_UTF_8_s_2_18, -1, 3, 0},
/* 19 */ { 4, irish_UTF_8_s_2_19, -1, 1, 0},
/* 20 */ { 7, irish_UTF_8_s_2_20, 19, 2, 0},
/* 21 */ { 5, irish_UTF_8_s_2_21, 19, 1, 0},
/* 22 */ { 11, irish_UTF_8_s_2_22, -1, 4, 0},
/* 23 */ { 10, irish_UTF_8_s_2_23, -1, 2, 0},
/* 24 */ { 14, irish_UTF_8_s_2_24, -1, 4, 0}
};

static const symbol irish_UTF_8_s_3_0[4] = { 'i', 'm', 'i', 'd' };
static const symbol irish_UTF_8_s_3_1[5] = { 'a', 'i', 'm', 'i', 'd' };
static const symbol irish_UTF_8_s_3_2[5] = { 0xC3, 0xAD, 'm', 'i', 'd' };
static const symbol irish_UTF_8_s_3_3[6] = { 'a', 0xC3, 0xAD, 'm', 'i', 'd' };
static const symbol irish_UTF_8_s_3_4[3] = { 'a', 'd', 'h' };
static const symbol irish_UTF_8_s_3_5[4] = { 'e', 'a', 'd', 'h' };
static const symbol irish_UTF_8_s_3_6[5] = { 'f', 'a', 'i', 'd', 'h' };
static const symbol irish_UTF_8_s_3_7[4] = { 'f', 'i', 'd', 'h' };
static const symbol irish_UTF_8_s_3_8[4] = { 0xC3, 0xA1, 'i', 'l' };
static const symbol irish_UTF_8_s_3_9[3] = { 'a', 'i', 'n' };
static const symbol irish_UTF_8_s_3_10[4] = { 't', 'e', 'a', 'r' };
static const symbol irish_UTF_8_s_3_11[3] = { 't', 'a', 'r' };

static const struct among irish_UTF_8_a_3[12] =
{
/*  0 */ { 4, irish_UTF_8_s_3_0, -1, 1, 0},
/*  1 */ { 5, irish_UTF_8_s_3_1, 0, 1, 0},
/*  2 */ { 5, irish_UTF_8_s_3_2, -1, 1, 0},
/*  3 */ { 6, irish_UTF_8_s_3_3, 2, 1, 0},
/*  4 */ { 3, irish_UTF_8_s_3_4, -1, 2, 0},
/*  5 */ { 4, irish_UTF_8_s_3_5, 4, 2, 0},
/*  6 */ { 5, irish_UTF_8_s_3_6, -1, 1, 0},
/*  7 */ { 4, irish_UTF_8_s_3_7, -1, 1, 0},
/*  8 */ { 4, irish_UTF_8_s_3_8, -1, 2, 0},
/*  9 */ { 3, irish_UTF_8_s_3_9, -1, 2, 0},
/* 10 */ { 4, irish_UTF_8_s_3_10, -1, 2, 0},
/* 11 */ { 3, irish_UTF_8_s_3_11, -1, 2, 0}
};

static const unsigned char irish_UTF_8_g_v[] = { 17, 65, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 17, 4, 2 };

static const symbol irish_UTF_8_s_0[] = { 'f' };
static const symbol irish_UTF_8_s_1[] = { 's' };
static const symbol irish_UTF_8_s_2[] = { 'b' };
static const symbol irish_UTF_8_s_3[] = { 'c' };
static const symbol irish_UTF_8_s_4[] = { 'd' };
static const symbol irish_UTF_8_s_5[] = { 'g' };
static const symbol irish_UTF_8_s_6[] = { 'p' };
static const symbol irish_UTF_8_s_7[] = { 't' };
static const symbol irish_UTF_8_s_8[] = { 'm' };
static const symbol irish_UTF_8_s_9[] = { 'a', 'r', 'c' };
static const symbol irish_UTF_8_s_10[] = { 'g', 'i', 'n' };
static const symbol irish_UTF_8_s_11[] = { 'g', 'r', 'a', 'f' };
static const symbol irish_UTF_8_s_12[] = { 'p', 'a', 'i', 't', 'e' };
static const symbol irish_UTF_8_s_13[] = { 0xC3, 0xB3, 'i', 'd' };

static int irish_UTF_8_r_mark_regions(struct SN_env * z) { /* forwardmode */
    z->I[0] = z->l; /* $pV = <integer expression>, line 30 */
    z->I[1] = z->l; /* $p1 = <integer expression>, line 31 */
    z->I[2] = z->l; /* $p2 = <integer expression>, line 32 */
    {   int c1 = z->c; /* do, line 34 */
        {    /* gopast */ /* grouping v, line 35 */
            int ret = out_grouping_U(z, irish_UTF_8_g_v, 97, 250, 1);
            if (ret < 0) goto lab0;
            z->c += ret;
        }
        z->I[0] = z->c; /* setmark pV, line 35 */
    lab0:
        z->c = c1;
    }
    {   int c2 = z->c; /* do, line 37 */
        {    /* gopast */ /* grouping v, line 38 */
            int ret = out_grouping_U(z, irish_UTF_8_g_v, 97, 250, 1);
            if (ret < 0) goto lab1;
            z->c += ret;
        }
        {    /* gopast */ /* non v, line 38 */
            int ret = in_grouping_U(z, irish_UTF_8_g_v, 97, 250, 1);
            if (ret < 0) goto lab1;
            z->c += ret;
        }
        z->I[1] = z->c; /* setmark p1, line 38 */
        {    /* gopast */ /* grouping v, line 39 */
            int ret = out_grouping_U(z, irish_UTF_8_g_v, 97, 250, 1);
            if (ret < 0) goto lab1;
            z->c += ret;
        }
        {    /* gopast */ /* non v, line 39 */
            int ret = in_grouping_U(z, irish_UTF_8_g_v, 97, 250, 1);
            if (ret < 0) goto lab1;
            z->c += ret;
        }
        z->I[2] = z->c; /* setmark p2, line 39 */
    lab1:
        z->c = c2;
    }
    return 1;
}

static int irish_UTF_8_r_initial_morph(struct SN_env * z) { /* forwardmode */
    int among_var;
    z->bra = z->c; /* [, line 44 */
    among_var = find_among(z, irish_UTF_8_a_0, 24); /* substring, line 44 */
    if (!(among_var)) return 0;
    z->ket = z->c; /* ], line 44 */
    switch (among_var) { /* among, line 44 */
        case 1:
            {   int ret = slice_del(z); /* delete, line 46 */
                if (ret < 0) return ret;
            }
            break;
        case 2:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_0); /* <-, line 52 */
                if (ret < 0) return ret;
            }
            break;
        case 3:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_1); /* <-, line 58 */
                if (ret < 0) return ret;
            }
            break;
        case 4:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_2); /* <-, line 61 */
                if (ret < 0) return ret;
            }
            break;
        case 5:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_3); /* <-, line 63 */
                if (ret < 0) return ret;
            }
            break;
        case 6:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_4); /* <-, line 65 */
                if (ret < 0) return ret;
            }
            break;
        case 7:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_5); /* <-, line 69 */
                if (ret < 0) return ret;
            }
            break;
        case 8:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_6); /* <-, line 71 */
                if (ret < 0) return ret;
            }
            break;
        case 9:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_7); /* <-, line 75 */
                if (ret < 0) return ret;
            }
            break;
        case 10:
            {   int ret = slice_from_s(z, 1, irish_UTF_8_s_8); /* <-, line 89 */
                if (ret < 0) return ret;
            }
            break;
    }
    return 1;
}

static int irish_UTF_8_r_RV(struct SN_env * z) { /* backwardmode */
    if (!(z->I[0] <= z->c)) return 0; /* $(<integer expression> <= <integer expression>), line 99 */
    return 1;
}

static int irish_UTF_8_r_R1(struct SN_env * z) { /* backwardmode */
    if (!(z->I[1] <= z->c)) return 0; /* $(<integer expression> <= <integer expression>), line 100 */
    return 1;
}

static int irish_UTF_8_r_R2(struct SN_env * z) { /* backwardmode */
    if (!(z->I[2] <= z->c)) return 0; /* $(<integer expression> <= <integer expression>), line 101 */
    return 1;
}

static int irish_UTF_8_r_noun_sfx(struct SN_env * z) { /* backwardmode */
    int among_var;
    z->ket = z->c; /* [, line 104 */
    among_var = find_among_b(z, irish_UTF_8_a_1, 16); /* substring, line 104 */
    if (!(among_var)) return 0;
    z->bra = z->c; /* ], line 104 */
    switch (among_var) { /* among, line 104 */
        case 1:
            {   int ret = irish_UTF_8_r_R1(z); /* call R1, line 108 */
                if (ret <= 0) return ret;
            }
            {   int ret = slice_del(z); /* delete, line 108 */
                if (ret < 0) return ret;
            }
            break;
        case 2:
            {   int ret = irish_UTF_8_r_R2(z); /* call R2, line 110 */
                if (ret <= 0) return ret;
            }
            {   int ret = slice_del(z); /* delete, line 110 */
                if (ret < 0) return ret;
            }
            break;
    }
    return 1;
}

static int irish_UTF_8_r_deriv(struct SN_env * z) { /* backwardmode */
    int among_var;
    z->ket = z->c; /* [, line 114 */
    among_var = find_among_b(z, irish_UTF_8_a_2, 25); /* substring, line 114 */
    if (!(among_var)) return 0;
    z->bra = z->c; /* ], line 114 */
    switch (among_var) { /* among, line 114 */
        case 1:
            {   int ret = irish_UTF_8_r_R2(z); /* call R2, line 116 */
                if (ret <= 0) return ret;
            }
            {   int ret = slice_del(z); /* delete, line 116 */
                if (ret < 0) return ret;
            }
            break;
        case 2:
            {   int ret = slice_from_s(z, 3, irish_UTF_8_s_9); /* <-, line 118 */
                if (ret < 0) return ret;
            }
            break;
        case 3:
            {   int ret = slice_from_s(z, 3, irish_UTF_8_s_10); /* <-, line 120 */
                if (ret < 0) return ret;
            }
            break;
        case 4:
            {   int ret = slice_from_s(z, 4, irish_UTF_8_s_11); /* <-, line 122 */
                if (ret < 0) return ret;
            }
            break;
        case 5:
            {   int ret = slice_from_s(z, 5, irish_UTF_8_s_12); /* <-, line 124 */
                if (ret < 0) return ret;
            }
            break;
        case 6:
            {   int ret = slice_from_s(z, 4, irish_UTF_8_s_13); /* <-, line 126 */
                if (ret < 0) return ret;
            }
            break;
    }
    return 1;
}

static int irish_UTF_8_r_verb_sfx(struct SN_env * z) { /* backwardmode */
    int among_var;
    z->ket = z->c; /* [, line 130 */
    if (z->c - 2 <= z->lb || z->p[z->c - 1] >> 5 != 3 || !((282896 >> (z->p[z->c - 1] & 0x1f)) & 1)) return 0; /* substring, line 130 */
    among_var = find_among_b(z, irish_UTF_8_a_3, 12);
    if (!(among_var)) return 0;
    z->bra = z->c; /* ], line 130 */
    switch (among_var) { /* among, line 130 */
        case 1:
            {   int ret = irish_UTF_8_r_RV(z); /* call RV, line 133 */
                if (ret <= 0) return ret;
            }
            {   int ret = slice_del(z); /* delete, line 133 */
                if (ret < 0) return ret;
            }
            break;
        case 2:
            {   int ret = irish_UTF_8_r_R1(z); /* call R1, line 138 */
                if (ret <= 0) return ret;
            }
            {   int ret = slice_del(z); /* delete, line 138 */
                if (ret < 0) return ret;
            }
            break;
    }
    return 1;
}

static int irish_UTF_8_stem(struct SN_env * z) { /* forwardmode */
    {   int c1 = z->c; /* do, line 144 */
        {   int ret = irish_UTF_8_r_initial_morph(z); /* call initial_morph, line 144 */
            if (ret == 0) goto lab0;
            if (ret < 0) return ret;
        }
    lab0:
        z->c = c1;
    }
    /* do, line 145 */
    {   int ret = irish_UTF_8_r_mark_regions(z); /* call mark_regions, line 145 */
        if (ret == 0) goto lab1;
        if (ret < 0) return ret;
    }
lab1:
    z->lb = z->c; z->c = z->l; /* backwards, line 146 */

    {   int m2 = z->l - z->c; (void)m2; /* do, line 147 */
        {   int ret = irish_UTF_8_r_noun_sfx(z); /* call noun_sfx, line 147 */
            if (ret == 0) goto lab2;
            if (ret < 0) return ret;
        }
    lab2:
        z->c = z->l - m2;
    }
    {   int m3 = z->l - z->c; (void)m3; /* do, line 148 */
        {   int ret = irish_UTF_8_r_deriv(z); /* call deriv, line 148 */
            if (ret == 0) goto lab3;
            if (ret < 0) return ret;
        }
    lab3:
        z->c = z->l - m3;
    }
    {   int m4 = z->l - z->c; (void)m4; /* do, line 149 */
        {   int ret = irish_UTF_8_r_verb_sfx(z); /* call verb_sfx, line 149 */
            if (ret == 0) goto lab4;
            if (ret < 0) return ret;
        }
    lab4:
        z->c = z->l - m4;
    }
    z->c = z->lb;
    return 1;
}

static struct SN_env * irish_UTF_8_create_env(struct SN_env *z) { return SN_create_env(z, 0, 3, 0); }

static void irish_UTF_8_close_env(struct SN_env * z) { SN_close_env(z, 0); }

