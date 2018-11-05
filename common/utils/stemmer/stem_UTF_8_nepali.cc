/* This file was generated automatically by the Snowball to ISO C compiler */
/* http://snowballstem.org/ */
static int nepali_UTF_8_stem(struct SN_env * z);
static int nepali_UTF_8_r_remove_category_3(struct SN_env * z);
static int nepali_UTF_8_r_remove_category_2(struct SN_env * z);
static int nepali_UTF_8_r_check_category_2(struct SN_env * z);
static int nepali_UTF_8_r_remove_category_1(struct SN_env * z);


static struct SN_env * nepali_UTF_8_create_env(struct SN_env *z);
static void nepali_UTF_8_close_env(struct SN_env * z);


static const symbol nepali_UTF_8_s_0_0[6] = { 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_0_1[9] = { 0xE0, 0xA4, 0xB2, 0xE0, 0xA4, 0xBE, 0xE0, 0xA4, 0x87 };
static const symbol nepali_UTF_8_s_0_2[6] = { 0xE0, 0xA4, 0xB2, 0xE0, 0xA5, 0x87 };
static const symbol nepali_UTF_8_s_0_3[9] = { 0xE0, 0xA4, 0xB2, 0xE0, 0xA4, 0xBE, 0xE0, 0xA4, 0x88 };
static const symbol nepali_UTF_8_s_0_4[6] = { 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_0_5[12] = { 0xE0, 0xA4, 0xB8, 0xE0, 0xA4, 0x81, 0xE0, 0xA4, 0x97, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_0_6[6] = { 0xE0, 0xA4, 0xAE, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_0_7[6] = { 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_0_8[9] = { 0xE0, 0xA4, 0xB8, 0xE0, 0xA4, 0x81, 0xE0, 0xA4, 0x97 };
static const symbol nepali_UTF_8_s_0_9[9] = { 0xE0, 0xA4, 0xB8, 0xE0, 0xA4, 0x82, 0xE0, 0xA4, 0x97 };
static const symbol nepali_UTF_8_s_0_10[18] = { 0xE0, 0xA4, 0xAE, 0xE0, 0xA4, 0xBE, 0xE0, 0xA4, 0xB0, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xAB, 0xE0, 0xA4, 0xA4 };
static const symbol nepali_UTF_8_s_0_11[6] = { 0xE0, 0xA4, 0xB0, 0xE0, 0xA4, 0xA4 };
static const symbol nepali_UTF_8_s_0_12[6] = { 0xE0, 0xA4, 0x95, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_0_13[6] = { 0xE0, 0xA4, 0xAE, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_0_14[18] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xB5, 0xE0, 0xA4, 0xBE, 0xE0, 0xA4, 0xB0, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_0_15[6] = { 0xE0, 0xA4, 0x95, 0xE0, 0xA4, 0xBF };
static const symbol nepali_UTF_8_s_0_16[9] = { 0xE0, 0xA4, 0xAA, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xBF };

static const struct among nepali_UTF_8_a_0[17] =
{
/*  0 */ { 6, nepali_UTF_8_s_0_0, -1, 2, 0},
/*  1 */ { 9, nepali_UTF_8_s_0_1, -1, 1, 0},
/*  2 */ { 6, nepali_UTF_8_s_0_2, -1, 1, 0},
/*  3 */ { 9, nepali_UTF_8_s_0_3, -1, 1, 0},
/*  4 */ { 6, nepali_UTF_8_s_0_4, -1, 2, 0},
/*  5 */ { 12, nepali_UTF_8_s_0_5, -1, 1, 0},
/*  6 */ { 6, nepali_UTF_8_s_0_6, -1, 1, 0},
/*  7 */ { 6, nepali_UTF_8_s_0_7, -1, 2, 0},
/*  8 */ { 9, nepali_UTF_8_s_0_8, -1, 1, 0},
/*  9 */ { 9, nepali_UTF_8_s_0_9, -1, 1, 0},
/* 10 */ { 18, nepali_UTF_8_s_0_10, -1, 1, 0},
/* 11 */ { 6, nepali_UTF_8_s_0_11, -1, 1, 0},
/* 12 */ { 6, nepali_UTF_8_s_0_12, -1, 2, 0},
/* 13 */ { 6, nepali_UTF_8_s_0_13, -1, 1, 0},
/* 14 */ { 18, nepali_UTF_8_s_0_14, -1, 1, 0},
/* 15 */ { 6, nepali_UTF_8_s_0_15, -1, 2, 0},
/* 16 */ { 9, nepali_UTF_8_s_0_16, -1, 1, 0}
};

static const symbol nepali_UTF_8_s_1_0[3] = { 0xE0, 0xA4, 0x81 };
static const symbol nepali_UTF_8_s_1_1[3] = { 0xE0, 0xA4, 0x82 };
static const symbol nepali_UTF_8_s_1_2[3] = { 0xE0, 0xA5, 0x88 };

static const struct among nepali_UTF_8_a_1[3] =
{
/*  0 */ { 3, nepali_UTF_8_s_1_0, -1, -1, 0},
/*  1 */ { 3, nepali_UTF_8_s_1_1, -1, -1, 0},
/*  2 */ { 3, nepali_UTF_8_s_1_2, -1, -1, 0}
};

static const symbol nepali_UTF_8_s_2_0[3] = { 0xE0, 0xA4, 0x81 };
static const symbol nepali_UTF_8_s_2_1[3] = { 0xE0, 0xA4, 0x82 };
static const symbol nepali_UTF_8_s_2_2[3] = { 0xE0, 0xA5, 0x88 };

static const struct among nepali_UTF_8_a_2[3] =
{
/*  0 */ { 3, nepali_UTF_8_s_2_0, -1, 1, 0},
/*  1 */ { 3, nepali_UTF_8_s_2_1, -1, 1, 0},
/*  2 */ { 3, nepali_UTF_8_s_2_2, -1, 2, 0}
};

static const symbol nepali_UTF_8_s_3_0[9] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_3_1[9] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_3_2[12] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_3_3[12] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_3_4[12] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x96, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_3_5[6] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_3_6[6] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x80 };
static const symbol nepali_UTF_8_s_3_7[6] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x81 };
static const symbol nepali_UTF_8_s_3_8[9] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x81 };
static const symbol nepali_UTF_8_s_3_9[12] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x81 };
static const symbol nepali_UTF_8_s_3_10[9] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x81 };
static const symbol nepali_UTF_8_s_3_11[6] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x81 };
static const symbol nepali_UTF_8_s_3_12[9] = { 0xE0, 0xA4, 0xB9, 0xE0, 0xA4, 0xB0, 0xE0, 0xA5, 0x81 };
static const symbol nepali_UTF_8_s_3_13[9] = { 0xE0, 0xA4, 0xB9, 0xE0, 0xA4, 0xB0, 0xE0, 0xA5, 0x82 };
static const symbol nepali_UTF_8_s_3_14[6] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x87 };
static const symbol nepali_UTF_8_s_3_15[6] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA5, 0x87 };
static const symbol nepali_UTF_8_s_3_16[6] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87 };
static const symbol nepali_UTF_8_s_3_17[9] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_3_18[12] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_3_19[9] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_3_20[6] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_3_21[9] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_3_22[9] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x88 };
static const symbol nepali_UTF_8_s_3_23[9] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_24[12] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_25[9] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_26[12] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_27[12] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_28[6] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_29[9] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_30[9] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_31[6] = { 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_32[9] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_33[12] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_34[9] = { 0xE0, 0xA4, 0xAD, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_35[9] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_36[12] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_37[12] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8B };
static const symbol nepali_UTF_8_s_3_38[6] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_39[9] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_40[9] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_41[12] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_42[9] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_43[9] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_44[6] = { 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_45[12] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_46[12] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_47[12] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3_48[9] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_49[12] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_50[12] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_51[15] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_52[12] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_53[12] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_54[12] = { 0xE0, 0xA4, 0xB2, 0xE0, 0xA4, 0xBE, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_55[12] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_56[12] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_57[9] = { 0xE0, 0xA4, 0xAA, 0xE0, 0xA4, 0xB0, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_58[9] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_59[15] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_60[12] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_61[12] = { 0xE0, 0xA4, 0xB9, 0xE0, 0xA5, 0x8B, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_62[9] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_63[12] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_64[12] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_65[15] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_66[12] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_67[12] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x9B, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_68[9] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_69[12] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xB8, 0xE0, 0xA5, 0x8D };
static const symbol nepali_UTF_8_s_3_70[9] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x8F };
static const symbol nepali_UTF_8_s_3_71[3] = { 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_72[6] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_73[6] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_74[9] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_75[15] = { 0xE0, 0xA4, 0xB9, 0xE0, 0xA5, 0x81, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_76[15] = { 0xE0, 0xA4, 0xB9, 0xE0, 0xA5, 0x81, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_77[12] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_78[12] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_79[6] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_80[6] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x9B };
static const symbol nepali_UTF_8_s_3_81[9] = { 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x95, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_82[12] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x95, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_83[9] = { 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_84[12] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_85[12] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0x8F, 0xE0, 0xA4, 0x95, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_86[6] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_87[9] = { 0xE0, 0xA4, 0x87, 0xE0, 0xA4, 0xA6, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_88[9] = { 0xE0, 0xA4, 0xBF, 0xE0, 0xA4, 0xA6, 0xE0, 0xA4, 0xBE };
static const symbol nepali_UTF_8_s_3_89[12] = { 0xE0, 0xA4, 0xA6, 0xE0, 0xA5, 0x87, 0xE0, 0xA4, 0x96, 0xE0, 0xA4, 0xBF };
static const symbol nepali_UTF_8_s_3_90[12] = { 0xE0, 0xA4, 0xAE, 0xE0, 0xA4, 0xBE, 0xE0, 0xA4, 0xA5, 0xE0, 0xA4, 0xBF };

static const struct among nepali_UTF_8_a_3[91] =
{
/*  0 */ { 9, nepali_UTF_8_s_3_0, -1, 1, 0},
/*  1 */ { 9, nepali_UTF_8_s_3_1, -1, 1, 0},
/*  2 */ { 12, nepali_UTF_8_s_3_2, 1, 1, 0},
/*  3 */ { 12, nepali_UTF_8_s_3_3, 1, 1, 0},
/*  4 */ { 12, nepali_UTF_8_s_3_4, -1, 1, 0},
/*  5 */ { 6, nepali_UTF_8_s_3_5, -1, 1, 0},
/*  6 */ { 6, nepali_UTF_8_s_3_6, -1, 1, 0},
/*  7 */ { 6, nepali_UTF_8_s_3_7, -1, 1, 0},
/*  8 */ { 9, nepali_UTF_8_s_3_8, 7, 1, 0},
/*  9 */ { 12, nepali_UTF_8_s_3_9, 8, 1, 0},
/* 10 */ { 9, nepali_UTF_8_s_3_10, 7, 1, 0},
/* 11 */ { 6, nepali_UTF_8_s_3_11, -1, 1, 0},
/* 12 */ { 9, nepali_UTF_8_s_3_12, -1, 1, 0},
/* 13 */ { 9, nepali_UTF_8_s_3_13, -1, 1, 0},
/* 14 */ { 6, nepali_UTF_8_s_3_14, -1, 1, 0},
/* 15 */ { 6, nepali_UTF_8_s_3_15, -1, 1, 0},
/* 16 */ { 6, nepali_UTF_8_s_3_16, -1, 1, 0},
/* 17 */ { 9, nepali_UTF_8_s_3_17, -1, 1, 0},
/* 18 */ { 12, nepali_UTF_8_s_3_18, 17, 1, 0},
/* 19 */ { 9, nepali_UTF_8_s_3_19, -1, 1, 0},
/* 20 */ { 6, nepali_UTF_8_s_3_20, -1, 1, 0},
/* 21 */ { 9, nepali_UTF_8_s_3_21, 20, 1, 0},
/* 22 */ { 9, nepali_UTF_8_s_3_22, 20, 1, 0},
/* 23 */ { 9, nepali_UTF_8_s_3_23, -1, 1, 0},
/* 24 */ { 12, nepali_UTF_8_s_3_24, 23, 1, 0},
/* 25 */ { 9, nepali_UTF_8_s_3_25, -1, 1, 0},
/* 26 */ { 12, nepali_UTF_8_s_3_26, 25, 1, 0},
/* 27 */ { 12, nepali_UTF_8_s_3_27, 25, 1, 0},
/* 28 */ { 6, nepali_UTF_8_s_3_28, -1, 1, 0},
/* 29 */ { 9, nepali_UTF_8_s_3_29, 28, 1, 0},
/* 30 */ { 9, nepali_UTF_8_s_3_30, 28, 1, 0},
/* 31 */ { 6, nepali_UTF_8_s_3_31, -1, 1, 0},
/* 32 */ { 9, nepali_UTF_8_s_3_32, 31, 1, 0},
/* 33 */ { 12, nepali_UTF_8_s_3_33, 31, 1, 0},
/* 34 */ { 9, nepali_UTF_8_s_3_34, 31, 1, 0},
/* 35 */ { 9, nepali_UTF_8_s_3_35, 31, 1, 0},
/* 36 */ { 12, nepali_UTF_8_s_3_36, 35, 1, 0},
/* 37 */ { 12, nepali_UTF_8_s_3_37, 35, 1, 0},
/* 38 */ { 6, nepali_UTF_8_s_3_38, -1, 1, 0},
/* 39 */ { 9, nepali_UTF_8_s_3_39, 38, 1, 0},
/* 40 */ { 9, nepali_UTF_8_s_3_40, 38, 1, 0},
/* 41 */ { 12, nepali_UTF_8_s_3_41, 40, 1, 0},
/* 42 */ { 9, nepali_UTF_8_s_3_42, 38, 1, 0},
/* 43 */ { 9, nepali_UTF_8_s_3_43, 38, 1, 0},
/* 44 */ { 6, nepali_UTF_8_s_3_44, -1, 1, 0},
/* 45 */ { 12, nepali_UTF_8_s_3_45, 44, 1, 0},
/* 46 */ { 12, nepali_UTF_8_s_3_46, 44, 1, 0},
/* 47 */ { 12, nepali_UTF_8_s_3_47, 44, 1, 0},
/* 48 */ { 9, nepali_UTF_8_s_3_48, -1, 1, 0},
/* 49 */ { 12, nepali_UTF_8_s_3_49, 48, 1, 0},
/* 50 */ { 12, nepali_UTF_8_s_3_50, 48, 1, 0},
/* 51 */ { 15, nepali_UTF_8_s_3_51, 50, 1, 0},
/* 52 */ { 12, nepali_UTF_8_s_3_52, 48, 1, 0},
/* 53 */ { 12, nepali_UTF_8_s_3_53, 48, 1, 0},
/* 54 */ { 12, nepali_UTF_8_s_3_54, -1, 1, 0},
/* 55 */ { 12, nepali_UTF_8_s_3_55, -1, 1, 0},
/* 56 */ { 12, nepali_UTF_8_s_3_56, -1, 1, 0},
/* 57 */ { 9, nepali_UTF_8_s_3_57, -1, 1, 0},
/* 58 */ { 9, nepali_UTF_8_s_3_58, -1, 1, 0},
/* 59 */ { 15, nepali_UTF_8_s_3_59, 58, 1, 0},
/* 60 */ { 12, nepali_UTF_8_s_3_60, -1, 1, 0},
/* 61 */ { 12, nepali_UTF_8_s_3_61, -1, 1, 0},
/* 62 */ { 9, nepali_UTF_8_s_3_62, -1, 1, 0},
/* 63 */ { 12, nepali_UTF_8_s_3_63, 62, 1, 0},
/* 64 */ { 12, nepali_UTF_8_s_3_64, 62, 1, 0},
/* 65 */ { 15, nepali_UTF_8_s_3_65, 64, 1, 0},
/* 66 */ { 12, nepali_UTF_8_s_3_66, 62, 1, 0},
/* 67 */ { 12, nepali_UTF_8_s_3_67, 62, 1, 0},
/* 68 */ { 9, nepali_UTF_8_s_3_68, -1, 1, 0},
/* 69 */ { 12, nepali_UTF_8_s_3_69, 68, 1, 0},
/* 70 */ { 9, nepali_UTF_8_s_3_70, -1, 1, 0},
/* 71 */ { 3, nepali_UTF_8_s_3_71, -1, 1, 0},
/* 72 */ { 6, nepali_UTF_8_s_3_72, 71, 1, 0},
/* 73 */ { 6, nepali_UTF_8_s_3_73, 71, 1, 0},
/* 74 */ { 9, nepali_UTF_8_s_3_74, 73, 1, 0},
/* 75 */ { 15, nepali_UTF_8_s_3_75, 74, 1, 0},
/* 76 */ { 15, nepali_UTF_8_s_3_76, 71, 1, 0},
/* 77 */ { 12, nepali_UTF_8_s_3_77, 71, 1, 0},
/* 78 */ { 12, nepali_UTF_8_s_3_78, 71, 1, 0},
/* 79 */ { 6, nepali_UTF_8_s_3_79, 71, 1, 0},
/* 80 */ { 6, nepali_UTF_8_s_3_80, 71, 1, 0},
/* 81 */ { 9, nepali_UTF_8_s_3_81, -1, 1, 0},
/* 82 */ { 12, nepali_UTF_8_s_3_82, 81, 1, 0},
/* 83 */ { 9, nepali_UTF_8_s_3_83, -1, 1, 0},
/* 84 */ { 12, nepali_UTF_8_s_3_84, 83, 1, 0},
/* 85 */ { 12, nepali_UTF_8_s_3_85, 83, 1, 0},
/* 86 */ { 6, nepali_UTF_8_s_3_86, -1, 1, 0},
/* 87 */ { 9, nepali_UTF_8_s_3_87, 86, 1, 0},
/* 88 */ { 9, nepali_UTF_8_s_3_88, 86, 1, 0},
/* 89 */ { 12, nepali_UTF_8_s_3_89, -1, 1, 0},
/* 90 */ { 12, nepali_UTF_8_s_3_90, -1, 1, 0}
};

static const symbol nepali_UTF_8_s_0[] = { 0xE0, 0xA4, 0x8F };
static const symbol nepali_UTF_8_s_1[] = { 0xE0, 0xA5, 0x87 };
static const symbol nepali_UTF_8_s_2[] = { 0xE0, 0xA4, 0xAF, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_3[] = { 0xE0, 0xA4, 0x9B, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_4[] = { 0xE0, 0xA4, 0xA8, 0xE0, 0xA5, 0x8C };
static const symbol nepali_UTF_8_s_5[] = { 0xE0, 0xA4, 0xA5, 0xE0, 0xA5, 0x87 };
static const symbol nepali_UTF_8_s_6[] = { 0xE0, 0xA4, 0xA4, 0xE0, 0xA5, 0x8D, 0xE0, 0xA4, 0xB0 };

static int nepali_UTF_8_r_remove_category_1(struct SN_env * z) { /* backwardmode */
    int among_var;
    z->ket = z->c; /* [, line 54 */
    among_var = find_among_b(z, nepali_UTF_8_a_0, 17); /* substring, line 54 */
    if (!(among_var)) return 0;
    z->bra = z->c; /* ], line 54 */
    switch (among_var) { /* among, line 54 */
        case 1:
            {   int ret = slice_del(z); /* delete, line 58 */
                if (ret < 0) return ret;
            }
            break;
        case 2:
            {   int m1 = z->l - z->c; (void)m1; /* or, line 59 */
                {   int m2 = z->l - z->c; (void)m2; /* or, line 59 */
                    if (!(eq_s_b(z, 3, nepali_UTF_8_s_0))) goto lab3; /* literal, line 59 */
                    goto lab2;
                lab3:
                    z->c = z->l - m2;
                    if (!(eq_s_b(z, 3, nepali_UTF_8_s_1))) goto lab1; /* literal, line 59 */
                }
            lab2:
                goto lab0;
            lab1:
                z->c = z->l - m1;
                {   int ret = slice_del(z); /* delete, line 59 */
                    if (ret < 0) return ret;
                }
            }
        lab0:
            break;
    }
    return 1;
}

static int nepali_UTF_8_r_check_category_2(struct SN_env * z) { /* backwardmode */
    z->ket = z->c; /* [, line 64 */
    if (z->c - 2 <= z->lb || z->p[z->c - 1] >> 5 != 4 || !((262 >> (z->p[z->c - 1] & 0x1f)) & 1)) return 0; /* substring, line 64 */
    if (!(find_among_b(z, nepali_UTF_8_a_1, 3))) return 0;
    z->bra = z->c; /* ], line 64 */
    return 1;
}

static int nepali_UTF_8_r_remove_category_2(struct SN_env * z) { /* backwardmode */
    int among_var;
    z->ket = z->c; /* [, line 70 */
    if (z->c - 2 <= z->lb || z->p[z->c - 1] >> 5 != 4 || !((262 >> (z->p[z->c - 1] & 0x1f)) & 1)) return 0; /* substring, line 70 */
    among_var = find_among_b(z, nepali_UTF_8_a_2, 3);
    if (!(among_var)) return 0;
    z->bra = z->c; /* ], line 70 */
    switch (among_var) { /* among, line 70 */
        case 1:
            {   int m1 = z->l - z->c; (void)m1; /* or, line 71 */
                if (!(eq_s_b(z, 6, nepali_UTF_8_s_2))) goto lab1; /* literal, line 71 */
                goto lab0;
            lab1:
                z->c = z->l - m1;
                if (!(eq_s_b(z, 6, nepali_UTF_8_s_3))) goto lab2; /* literal, line 71 */
                goto lab0;
            lab2:
                z->c = z->l - m1;
                if (!(eq_s_b(z, 6, nepali_UTF_8_s_4))) goto lab3; /* literal, line 71 */
                goto lab0;
            lab3:
                z->c = z->l - m1;
                if (!(eq_s_b(z, 6, nepali_UTF_8_s_5))) return 0; /* literal, line 71 */
            }
        lab0:
            {   int ret = slice_del(z); /* delete, line 71 */
                if (ret < 0) return ret;
            }
            break;
        case 2:
            if (!(eq_s_b(z, 9, nepali_UTF_8_s_6))) return 0; /* literal, line 72 */
            {   int ret = slice_del(z); /* delete, line 72 */
                if (ret < 0) return ret;
            }
            break;
    }
    return 1;
}

static int nepali_UTF_8_r_remove_category_3(struct SN_env * z) { /* backwardmode */
    z->ket = z->c; /* [, line 77 */
    if (!(find_among_b(z, nepali_UTF_8_a_3, 91))) return 0; /* substring, line 77 */
    z->bra = z->c; /* ], line 77 */
    {   int ret = slice_del(z); /* delete, line 79 */
        if (ret < 0) return ret;
    }
    return 1;
}

static int nepali_UTF_8_stem(struct SN_env * z) { /* forwardmode */
    z->lb = z->c; z->c = z->l; /* backwards, line 86 */

    {   int m1 = z->l - z->c; (void)m1; /* do, line 87 */
        {   int ret = nepali_UTF_8_r_remove_category_1(z); /* call remove_category_1, line 87 */
            if (ret == 0) goto lab0;
            if (ret < 0) return ret;
        }
    lab0:
        z->c = z->l - m1;
    }
    {   int m2 = z->l - z->c; (void)m2; /* do, line 88 */
        while(1) { /* repeat, line 89 */
            int m3 = z->l - z->c; (void)m3;
            {   int m4 = z->l - z->c; (void)m4; /* do, line 89 */
                {   int m5 = z->l - z->c; (void)m5; /* and, line 89 */
                    {   int ret = nepali_UTF_8_r_check_category_2(z); /* call check_category_2, line 89 */
                        if (ret == 0) goto lab3;
                        if (ret < 0) return ret;
                    }
                    z->c = z->l - m5;
                    {   int ret = nepali_UTF_8_r_remove_category_2(z); /* call remove_category_2, line 89 */
                        if (ret == 0) goto lab3;
                        if (ret < 0) return ret;
                    }
                }
            lab3:
                z->c = z->l - m4;
            }
            {   int ret = nepali_UTF_8_r_remove_category_3(z); /* call remove_category_3, line 89 */
                if (ret == 0) goto lab2;
                if (ret < 0) return ret;
            }
            continue;
        lab2:
            z->c = z->l - m3;
            break;
        }
        z->c = z->l - m2;
    }
    z->c = z->lb;
    return 1;
}

static struct SN_env * nepali_UTF_8_create_env(struct SN_env *z) { return SN_create_env(z, 0, 0, 0); }

static void nepali_UTF_8_close_env(struct SN_env * z) { SN_close_env(z, 0); }

