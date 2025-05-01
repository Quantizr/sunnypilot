#include "pose.h"

namespace {
#define DIM 18
#define EDIM 18
#define MEDIM 18
typedef void (*Hfun)(double *, double *, double *);
const static double MAHA_THRESH_4 = 7.814727903251177;
const static double MAHA_THRESH_10 = 7.814727903251177;
const static double MAHA_THRESH_13 = 7.814727903251177;
const static double MAHA_THRESH_14 = 7.814727903251177;

/******************************************************************************
 *                      Code generated with SymPy 1.13.2                      *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_565953791747580906) {
   out_565953791747580906[0] = delta_x[0] + nom_x[0];
   out_565953791747580906[1] = delta_x[1] + nom_x[1];
   out_565953791747580906[2] = delta_x[2] + nom_x[2];
   out_565953791747580906[3] = delta_x[3] + nom_x[3];
   out_565953791747580906[4] = delta_x[4] + nom_x[4];
   out_565953791747580906[5] = delta_x[5] + nom_x[5];
   out_565953791747580906[6] = delta_x[6] + nom_x[6];
   out_565953791747580906[7] = delta_x[7] + nom_x[7];
   out_565953791747580906[8] = delta_x[8] + nom_x[8];
   out_565953791747580906[9] = delta_x[9] + nom_x[9];
   out_565953791747580906[10] = delta_x[10] + nom_x[10];
   out_565953791747580906[11] = delta_x[11] + nom_x[11];
   out_565953791747580906[12] = delta_x[12] + nom_x[12];
   out_565953791747580906[13] = delta_x[13] + nom_x[13];
   out_565953791747580906[14] = delta_x[14] + nom_x[14];
   out_565953791747580906[15] = delta_x[15] + nom_x[15];
   out_565953791747580906[16] = delta_x[16] + nom_x[16];
   out_565953791747580906[17] = delta_x[17] + nom_x[17];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_5141089249990947268) {
   out_5141089249990947268[0] = -nom_x[0] + true_x[0];
   out_5141089249990947268[1] = -nom_x[1] + true_x[1];
   out_5141089249990947268[2] = -nom_x[2] + true_x[2];
   out_5141089249990947268[3] = -nom_x[3] + true_x[3];
   out_5141089249990947268[4] = -nom_x[4] + true_x[4];
   out_5141089249990947268[5] = -nom_x[5] + true_x[5];
   out_5141089249990947268[6] = -nom_x[6] + true_x[6];
   out_5141089249990947268[7] = -nom_x[7] + true_x[7];
   out_5141089249990947268[8] = -nom_x[8] + true_x[8];
   out_5141089249990947268[9] = -nom_x[9] + true_x[9];
   out_5141089249990947268[10] = -nom_x[10] + true_x[10];
   out_5141089249990947268[11] = -nom_x[11] + true_x[11];
   out_5141089249990947268[12] = -nom_x[12] + true_x[12];
   out_5141089249990947268[13] = -nom_x[13] + true_x[13];
   out_5141089249990947268[14] = -nom_x[14] + true_x[14];
   out_5141089249990947268[15] = -nom_x[15] + true_x[15];
   out_5141089249990947268[16] = -nom_x[16] + true_x[16];
   out_5141089249990947268[17] = -nom_x[17] + true_x[17];
}
void H_mod_fun(double *state, double *out_8531364752282097876) {
   out_8531364752282097876[0] = 1.0;
   out_8531364752282097876[1] = 0.0;
   out_8531364752282097876[2] = 0.0;
   out_8531364752282097876[3] = 0.0;
   out_8531364752282097876[4] = 0.0;
   out_8531364752282097876[5] = 0.0;
   out_8531364752282097876[6] = 0.0;
   out_8531364752282097876[7] = 0.0;
   out_8531364752282097876[8] = 0.0;
   out_8531364752282097876[9] = 0.0;
   out_8531364752282097876[10] = 0.0;
   out_8531364752282097876[11] = 0.0;
   out_8531364752282097876[12] = 0.0;
   out_8531364752282097876[13] = 0.0;
   out_8531364752282097876[14] = 0.0;
   out_8531364752282097876[15] = 0.0;
   out_8531364752282097876[16] = 0.0;
   out_8531364752282097876[17] = 0.0;
   out_8531364752282097876[18] = 0.0;
   out_8531364752282097876[19] = 1.0;
   out_8531364752282097876[20] = 0.0;
   out_8531364752282097876[21] = 0.0;
   out_8531364752282097876[22] = 0.0;
   out_8531364752282097876[23] = 0.0;
   out_8531364752282097876[24] = 0.0;
   out_8531364752282097876[25] = 0.0;
   out_8531364752282097876[26] = 0.0;
   out_8531364752282097876[27] = 0.0;
   out_8531364752282097876[28] = 0.0;
   out_8531364752282097876[29] = 0.0;
   out_8531364752282097876[30] = 0.0;
   out_8531364752282097876[31] = 0.0;
   out_8531364752282097876[32] = 0.0;
   out_8531364752282097876[33] = 0.0;
   out_8531364752282097876[34] = 0.0;
   out_8531364752282097876[35] = 0.0;
   out_8531364752282097876[36] = 0.0;
   out_8531364752282097876[37] = 0.0;
   out_8531364752282097876[38] = 1.0;
   out_8531364752282097876[39] = 0.0;
   out_8531364752282097876[40] = 0.0;
   out_8531364752282097876[41] = 0.0;
   out_8531364752282097876[42] = 0.0;
   out_8531364752282097876[43] = 0.0;
   out_8531364752282097876[44] = 0.0;
   out_8531364752282097876[45] = 0.0;
   out_8531364752282097876[46] = 0.0;
   out_8531364752282097876[47] = 0.0;
   out_8531364752282097876[48] = 0.0;
   out_8531364752282097876[49] = 0.0;
   out_8531364752282097876[50] = 0.0;
   out_8531364752282097876[51] = 0.0;
   out_8531364752282097876[52] = 0.0;
   out_8531364752282097876[53] = 0.0;
   out_8531364752282097876[54] = 0.0;
   out_8531364752282097876[55] = 0.0;
   out_8531364752282097876[56] = 0.0;
   out_8531364752282097876[57] = 1.0;
   out_8531364752282097876[58] = 0.0;
   out_8531364752282097876[59] = 0.0;
   out_8531364752282097876[60] = 0.0;
   out_8531364752282097876[61] = 0.0;
   out_8531364752282097876[62] = 0.0;
   out_8531364752282097876[63] = 0.0;
   out_8531364752282097876[64] = 0.0;
   out_8531364752282097876[65] = 0.0;
   out_8531364752282097876[66] = 0.0;
   out_8531364752282097876[67] = 0.0;
   out_8531364752282097876[68] = 0.0;
   out_8531364752282097876[69] = 0.0;
   out_8531364752282097876[70] = 0.0;
   out_8531364752282097876[71] = 0.0;
   out_8531364752282097876[72] = 0.0;
   out_8531364752282097876[73] = 0.0;
   out_8531364752282097876[74] = 0.0;
   out_8531364752282097876[75] = 0.0;
   out_8531364752282097876[76] = 1.0;
   out_8531364752282097876[77] = 0.0;
   out_8531364752282097876[78] = 0.0;
   out_8531364752282097876[79] = 0.0;
   out_8531364752282097876[80] = 0.0;
   out_8531364752282097876[81] = 0.0;
   out_8531364752282097876[82] = 0.0;
   out_8531364752282097876[83] = 0.0;
   out_8531364752282097876[84] = 0.0;
   out_8531364752282097876[85] = 0.0;
   out_8531364752282097876[86] = 0.0;
   out_8531364752282097876[87] = 0.0;
   out_8531364752282097876[88] = 0.0;
   out_8531364752282097876[89] = 0.0;
   out_8531364752282097876[90] = 0.0;
   out_8531364752282097876[91] = 0.0;
   out_8531364752282097876[92] = 0.0;
   out_8531364752282097876[93] = 0.0;
   out_8531364752282097876[94] = 0.0;
   out_8531364752282097876[95] = 1.0;
   out_8531364752282097876[96] = 0.0;
   out_8531364752282097876[97] = 0.0;
   out_8531364752282097876[98] = 0.0;
   out_8531364752282097876[99] = 0.0;
   out_8531364752282097876[100] = 0.0;
   out_8531364752282097876[101] = 0.0;
   out_8531364752282097876[102] = 0.0;
   out_8531364752282097876[103] = 0.0;
   out_8531364752282097876[104] = 0.0;
   out_8531364752282097876[105] = 0.0;
   out_8531364752282097876[106] = 0.0;
   out_8531364752282097876[107] = 0.0;
   out_8531364752282097876[108] = 0.0;
   out_8531364752282097876[109] = 0.0;
   out_8531364752282097876[110] = 0.0;
   out_8531364752282097876[111] = 0.0;
   out_8531364752282097876[112] = 0.0;
   out_8531364752282097876[113] = 0.0;
   out_8531364752282097876[114] = 1.0;
   out_8531364752282097876[115] = 0.0;
   out_8531364752282097876[116] = 0.0;
   out_8531364752282097876[117] = 0.0;
   out_8531364752282097876[118] = 0.0;
   out_8531364752282097876[119] = 0.0;
   out_8531364752282097876[120] = 0.0;
   out_8531364752282097876[121] = 0.0;
   out_8531364752282097876[122] = 0.0;
   out_8531364752282097876[123] = 0.0;
   out_8531364752282097876[124] = 0.0;
   out_8531364752282097876[125] = 0.0;
   out_8531364752282097876[126] = 0.0;
   out_8531364752282097876[127] = 0.0;
   out_8531364752282097876[128] = 0.0;
   out_8531364752282097876[129] = 0.0;
   out_8531364752282097876[130] = 0.0;
   out_8531364752282097876[131] = 0.0;
   out_8531364752282097876[132] = 0.0;
   out_8531364752282097876[133] = 1.0;
   out_8531364752282097876[134] = 0.0;
   out_8531364752282097876[135] = 0.0;
   out_8531364752282097876[136] = 0.0;
   out_8531364752282097876[137] = 0.0;
   out_8531364752282097876[138] = 0.0;
   out_8531364752282097876[139] = 0.0;
   out_8531364752282097876[140] = 0.0;
   out_8531364752282097876[141] = 0.0;
   out_8531364752282097876[142] = 0.0;
   out_8531364752282097876[143] = 0.0;
   out_8531364752282097876[144] = 0.0;
   out_8531364752282097876[145] = 0.0;
   out_8531364752282097876[146] = 0.0;
   out_8531364752282097876[147] = 0.0;
   out_8531364752282097876[148] = 0.0;
   out_8531364752282097876[149] = 0.0;
   out_8531364752282097876[150] = 0.0;
   out_8531364752282097876[151] = 0.0;
   out_8531364752282097876[152] = 1.0;
   out_8531364752282097876[153] = 0.0;
   out_8531364752282097876[154] = 0.0;
   out_8531364752282097876[155] = 0.0;
   out_8531364752282097876[156] = 0.0;
   out_8531364752282097876[157] = 0.0;
   out_8531364752282097876[158] = 0.0;
   out_8531364752282097876[159] = 0.0;
   out_8531364752282097876[160] = 0.0;
   out_8531364752282097876[161] = 0.0;
   out_8531364752282097876[162] = 0.0;
   out_8531364752282097876[163] = 0.0;
   out_8531364752282097876[164] = 0.0;
   out_8531364752282097876[165] = 0.0;
   out_8531364752282097876[166] = 0.0;
   out_8531364752282097876[167] = 0.0;
   out_8531364752282097876[168] = 0.0;
   out_8531364752282097876[169] = 0.0;
   out_8531364752282097876[170] = 0.0;
   out_8531364752282097876[171] = 1.0;
   out_8531364752282097876[172] = 0.0;
   out_8531364752282097876[173] = 0.0;
   out_8531364752282097876[174] = 0.0;
   out_8531364752282097876[175] = 0.0;
   out_8531364752282097876[176] = 0.0;
   out_8531364752282097876[177] = 0.0;
   out_8531364752282097876[178] = 0.0;
   out_8531364752282097876[179] = 0.0;
   out_8531364752282097876[180] = 0.0;
   out_8531364752282097876[181] = 0.0;
   out_8531364752282097876[182] = 0.0;
   out_8531364752282097876[183] = 0.0;
   out_8531364752282097876[184] = 0.0;
   out_8531364752282097876[185] = 0.0;
   out_8531364752282097876[186] = 0.0;
   out_8531364752282097876[187] = 0.0;
   out_8531364752282097876[188] = 0.0;
   out_8531364752282097876[189] = 0.0;
   out_8531364752282097876[190] = 1.0;
   out_8531364752282097876[191] = 0.0;
   out_8531364752282097876[192] = 0.0;
   out_8531364752282097876[193] = 0.0;
   out_8531364752282097876[194] = 0.0;
   out_8531364752282097876[195] = 0.0;
   out_8531364752282097876[196] = 0.0;
   out_8531364752282097876[197] = 0.0;
   out_8531364752282097876[198] = 0.0;
   out_8531364752282097876[199] = 0.0;
   out_8531364752282097876[200] = 0.0;
   out_8531364752282097876[201] = 0.0;
   out_8531364752282097876[202] = 0.0;
   out_8531364752282097876[203] = 0.0;
   out_8531364752282097876[204] = 0.0;
   out_8531364752282097876[205] = 0.0;
   out_8531364752282097876[206] = 0.0;
   out_8531364752282097876[207] = 0.0;
   out_8531364752282097876[208] = 0.0;
   out_8531364752282097876[209] = 1.0;
   out_8531364752282097876[210] = 0.0;
   out_8531364752282097876[211] = 0.0;
   out_8531364752282097876[212] = 0.0;
   out_8531364752282097876[213] = 0.0;
   out_8531364752282097876[214] = 0.0;
   out_8531364752282097876[215] = 0.0;
   out_8531364752282097876[216] = 0.0;
   out_8531364752282097876[217] = 0.0;
   out_8531364752282097876[218] = 0.0;
   out_8531364752282097876[219] = 0.0;
   out_8531364752282097876[220] = 0.0;
   out_8531364752282097876[221] = 0.0;
   out_8531364752282097876[222] = 0.0;
   out_8531364752282097876[223] = 0.0;
   out_8531364752282097876[224] = 0.0;
   out_8531364752282097876[225] = 0.0;
   out_8531364752282097876[226] = 0.0;
   out_8531364752282097876[227] = 0.0;
   out_8531364752282097876[228] = 1.0;
   out_8531364752282097876[229] = 0.0;
   out_8531364752282097876[230] = 0.0;
   out_8531364752282097876[231] = 0.0;
   out_8531364752282097876[232] = 0.0;
   out_8531364752282097876[233] = 0.0;
   out_8531364752282097876[234] = 0.0;
   out_8531364752282097876[235] = 0.0;
   out_8531364752282097876[236] = 0.0;
   out_8531364752282097876[237] = 0.0;
   out_8531364752282097876[238] = 0.0;
   out_8531364752282097876[239] = 0.0;
   out_8531364752282097876[240] = 0.0;
   out_8531364752282097876[241] = 0.0;
   out_8531364752282097876[242] = 0.0;
   out_8531364752282097876[243] = 0.0;
   out_8531364752282097876[244] = 0.0;
   out_8531364752282097876[245] = 0.0;
   out_8531364752282097876[246] = 0.0;
   out_8531364752282097876[247] = 1.0;
   out_8531364752282097876[248] = 0.0;
   out_8531364752282097876[249] = 0.0;
   out_8531364752282097876[250] = 0.0;
   out_8531364752282097876[251] = 0.0;
   out_8531364752282097876[252] = 0.0;
   out_8531364752282097876[253] = 0.0;
   out_8531364752282097876[254] = 0.0;
   out_8531364752282097876[255] = 0.0;
   out_8531364752282097876[256] = 0.0;
   out_8531364752282097876[257] = 0.0;
   out_8531364752282097876[258] = 0.0;
   out_8531364752282097876[259] = 0.0;
   out_8531364752282097876[260] = 0.0;
   out_8531364752282097876[261] = 0.0;
   out_8531364752282097876[262] = 0.0;
   out_8531364752282097876[263] = 0.0;
   out_8531364752282097876[264] = 0.0;
   out_8531364752282097876[265] = 0.0;
   out_8531364752282097876[266] = 1.0;
   out_8531364752282097876[267] = 0.0;
   out_8531364752282097876[268] = 0.0;
   out_8531364752282097876[269] = 0.0;
   out_8531364752282097876[270] = 0.0;
   out_8531364752282097876[271] = 0.0;
   out_8531364752282097876[272] = 0.0;
   out_8531364752282097876[273] = 0.0;
   out_8531364752282097876[274] = 0.0;
   out_8531364752282097876[275] = 0.0;
   out_8531364752282097876[276] = 0.0;
   out_8531364752282097876[277] = 0.0;
   out_8531364752282097876[278] = 0.0;
   out_8531364752282097876[279] = 0.0;
   out_8531364752282097876[280] = 0.0;
   out_8531364752282097876[281] = 0.0;
   out_8531364752282097876[282] = 0.0;
   out_8531364752282097876[283] = 0.0;
   out_8531364752282097876[284] = 0.0;
   out_8531364752282097876[285] = 1.0;
   out_8531364752282097876[286] = 0.0;
   out_8531364752282097876[287] = 0.0;
   out_8531364752282097876[288] = 0.0;
   out_8531364752282097876[289] = 0.0;
   out_8531364752282097876[290] = 0.0;
   out_8531364752282097876[291] = 0.0;
   out_8531364752282097876[292] = 0.0;
   out_8531364752282097876[293] = 0.0;
   out_8531364752282097876[294] = 0.0;
   out_8531364752282097876[295] = 0.0;
   out_8531364752282097876[296] = 0.0;
   out_8531364752282097876[297] = 0.0;
   out_8531364752282097876[298] = 0.0;
   out_8531364752282097876[299] = 0.0;
   out_8531364752282097876[300] = 0.0;
   out_8531364752282097876[301] = 0.0;
   out_8531364752282097876[302] = 0.0;
   out_8531364752282097876[303] = 0.0;
   out_8531364752282097876[304] = 1.0;
   out_8531364752282097876[305] = 0.0;
   out_8531364752282097876[306] = 0.0;
   out_8531364752282097876[307] = 0.0;
   out_8531364752282097876[308] = 0.0;
   out_8531364752282097876[309] = 0.0;
   out_8531364752282097876[310] = 0.0;
   out_8531364752282097876[311] = 0.0;
   out_8531364752282097876[312] = 0.0;
   out_8531364752282097876[313] = 0.0;
   out_8531364752282097876[314] = 0.0;
   out_8531364752282097876[315] = 0.0;
   out_8531364752282097876[316] = 0.0;
   out_8531364752282097876[317] = 0.0;
   out_8531364752282097876[318] = 0.0;
   out_8531364752282097876[319] = 0.0;
   out_8531364752282097876[320] = 0.0;
   out_8531364752282097876[321] = 0.0;
   out_8531364752282097876[322] = 0.0;
   out_8531364752282097876[323] = 1.0;
}
void f_fun(double *state, double dt, double *out_8938703759442158393) {
   out_8938703759442158393[0] = atan2((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), -(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]));
   out_8938703759442158393[1] = asin(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]));
   out_8938703759442158393[2] = atan2(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), -(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]));
   out_8938703759442158393[3] = dt*state[12] + state[3];
   out_8938703759442158393[4] = dt*state[13] + state[4];
   out_8938703759442158393[5] = dt*state[14] + state[5];
   out_8938703759442158393[6] = state[6];
   out_8938703759442158393[7] = state[7];
   out_8938703759442158393[8] = state[8];
   out_8938703759442158393[9] = state[9];
   out_8938703759442158393[10] = state[10];
   out_8938703759442158393[11] = state[11];
   out_8938703759442158393[12] = state[12];
   out_8938703759442158393[13] = state[13];
   out_8938703759442158393[14] = state[14];
   out_8938703759442158393[15] = state[15];
   out_8938703759442158393[16] = state[16];
   out_8938703759442158393[17] = state[17];
}
void F_fun(double *state, double dt, double *out_7720863545457369091) {
   out_7720863545457369091[0] = ((-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*cos(state[0])*cos(state[1]) - sin(state[0])*cos(dt*state[6])*cos(dt*state[7])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + ((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*cos(state[0])*cos(state[1]) - sin(dt*state[6])*sin(state[0])*cos(dt*state[7])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7720863545457369091[1] = ((-sin(dt*state[6])*sin(dt*state[8]) - sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*cos(state[1]) - (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*sin(state[1]) - sin(state[1])*cos(dt*state[6])*cos(dt*state[7])*cos(state[0]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*sin(state[1]) + (-sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) + sin(dt*state[8])*cos(dt*state[6]))*cos(state[1]) - sin(dt*state[6])*sin(state[1])*cos(dt*state[7])*cos(state[0]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7720863545457369091[2] = 0;
   out_7720863545457369091[3] = 0;
   out_7720863545457369091[4] = 0;
   out_7720863545457369091[5] = 0;
   out_7720863545457369091[6] = (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(dt*cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*sin(dt*state[8]) - dt*sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-dt*sin(dt*state[6])*cos(dt*state[8]) + dt*sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) - dt*cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (dt*sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7720863545457369091[7] = (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[6])*sin(dt*state[7])*cos(state[0])*cos(state[1]) + dt*sin(dt*state[6])*sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) - dt*sin(dt*state[6])*sin(state[1])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[7])*cos(dt*state[6])*cos(state[0])*cos(state[1]) + dt*sin(dt*state[8])*sin(state[0])*cos(dt*state[6])*cos(dt*state[7])*cos(state[1]) - dt*sin(state[1])*cos(dt*state[6])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7720863545457369091[8] = ((dt*sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + dt*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (dt*sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + ((dt*sin(dt*state[6])*sin(dt*state[8]) + dt*sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*cos(dt*state[8]) + dt*sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_7720863545457369091[9] = 0;
   out_7720863545457369091[10] = 0;
   out_7720863545457369091[11] = 0;
   out_7720863545457369091[12] = 0;
   out_7720863545457369091[13] = 0;
   out_7720863545457369091[14] = 0;
   out_7720863545457369091[15] = 0;
   out_7720863545457369091[16] = 0;
   out_7720863545457369091[17] = 0;
   out_7720863545457369091[18] = (-sin(dt*state[7])*sin(state[0])*cos(state[1]) - sin(dt*state[8])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7720863545457369091[19] = (-sin(dt*state[7])*sin(state[1])*cos(state[0]) + sin(dt*state[8])*sin(state[0])*sin(state[1])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7720863545457369091[20] = 0;
   out_7720863545457369091[21] = 0;
   out_7720863545457369091[22] = 0;
   out_7720863545457369091[23] = 0;
   out_7720863545457369091[24] = 0;
   out_7720863545457369091[25] = (dt*sin(dt*state[7])*sin(dt*state[8])*sin(state[0])*cos(state[1]) - dt*sin(dt*state[7])*sin(state[1])*cos(dt*state[8]) + dt*cos(dt*state[7])*cos(state[0])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7720863545457369091[26] = (-dt*sin(dt*state[8])*sin(state[1])*cos(dt*state[7]) - dt*sin(state[0])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_7720863545457369091[27] = 0;
   out_7720863545457369091[28] = 0;
   out_7720863545457369091[29] = 0;
   out_7720863545457369091[30] = 0;
   out_7720863545457369091[31] = 0;
   out_7720863545457369091[32] = 0;
   out_7720863545457369091[33] = 0;
   out_7720863545457369091[34] = 0;
   out_7720863545457369091[35] = 0;
   out_7720863545457369091[36] = ((sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[7]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[7]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7720863545457369091[37] = (-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(-sin(dt*state[7])*sin(state[2])*cos(state[0])*cos(state[1]) + sin(dt*state[8])*sin(state[0])*sin(state[2])*cos(dt*state[7])*cos(state[1]) - sin(state[1])*sin(state[2])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*(-sin(dt*state[7])*cos(state[0])*cos(state[1])*cos(state[2]) + sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1])*cos(state[2]) - sin(state[1])*cos(dt*state[7])*cos(dt*state[8])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7720863545457369091[38] = ((-sin(state[0])*sin(state[2]) - sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (-sin(state[0])*sin(state[1])*sin(state[2]) - cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7720863545457369091[39] = 0;
   out_7720863545457369091[40] = 0;
   out_7720863545457369091[41] = 0;
   out_7720863545457369091[42] = 0;
   out_7720863545457369091[43] = (-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(dt*(sin(state[0])*cos(state[2]) - sin(state[1])*sin(state[2])*cos(state[0]))*cos(dt*state[7]) - dt*(sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[7])*sin(dt*state[8]) - dt*sin(dt*state[7])*sin(state[2])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*(dt*(-sin(state[0])*sin(state[2]) - sin(state[1])*cos(state[0])*cos(state[2]))*cos(dt*state[7]) - dt*(sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[7])*sin(dt*state[8]) - dt*sin(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7720863545457369091[44] = (dt*(sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*cos(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*sin(state[2])*cos(dt*state[7])*cos(state[1]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + (dt*(sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*cos(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[7])*cos(state[1])*cos(state[2]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_7720863545457369091[45] = 0;
   out_7720863545457369091[46] = 0;
   out_7720863545457369091[47] = 0;
   out_7720863545457369091[48] = 0;
   out_7720863545457369091[49] = 0;
   out_7720863545457369091[50] = 0;
   out_7720863545457369091[51] = 0;
   out_7720863545457369091[52] = 0;
   out_7720863545457369091[53] = 0;
   out_7720863545457369091[54] = 0;
   out_7720863545457369091[55] = 0;
   out_7720863545457369091[56] = 0;
   out_7720863545457369091[57] = 1;
   out_7720863545457369091[58] = 0;
   out_7720863545457369091[59] = 0;
   out_7720863545457369091[60] = 0;
   out_7720863545457369091[61] = 0;
   out_7720863545457369091[62] = 0;
   out_7720863545457369091[63] = 0;
   out_7720863545457369091[64] = 0;
   out_7720863545457369091[65] = 0;
   out_7720863545457369091[66] = dt;
   out_7720863545457369091[67] = 0;
   out_7720863545457369091[68] = 0;
   out_7720863545457369091[69] = 0;
   out_7720863545457369091[70] = 0;
   out_7720863545457369091[71] = 0;
   out_7720863545457369091[72] = 0;
   out_7720863545457369091[73] = 0;
   out_7720863545457369091[74] = 0;
   out_7720863545457369091[75] = 0;
   out_7720863545457369091[76] = 1;
   out_7720863545457369091[77] = 0;
   out_7720863545457369091[78] = 0;
   out_7720863545457369091[79] = 0;
   out_7720863545457369091[80] = 0;
   out_7720863545457369091[81] = 0;
   out_7720863545457369091[82] = 0;
   out_7720863545457369091[83] = 0;
   out_7720863545457369091[84] = 0;
   out_7720863545457369091[85] = dt;
   out_7720863545457369091[86] = 0;
   out_7720863545457369091[87] = 0;
   out_7720863545457369091[88] = 0;
   out_7720863545457369091[89] = 0;
   out_7720863545457369091[90] = 0;
   out_7720863545457369091[91] = 0;
   out_7720863545457369091[92] = 0;
   out_7720863545457369091[93] = 0;
   out_7720863545457369091[94] = 0;
   out_7720863545457369091[95] = 1;
   out_7720863545457369091[96] = 0;
   out_7720863545457369091[97] = 0;
   out_7720863545457369091[98] = 0;
   out_7720863545457369091[99] = 0;
   out_7720863545457369091[100] = 0;
   out_7720863545457369091[101] = 0;
   out_7720863545457369091[102] = 0;
   out_7720863545457369091[103] = 0;
   out_7720863545457369091[104] = dt;
   out_7720863545457369091[105] = 0;
   out_7720863545457369091[106] = 0;
   out_7720863545457369091[107] = 0;
   out_7720863545457369091[108] = 0;
   out_7720863545457369091[109] = 0;
   out_7720863545457369091[110] = 0;
   out_7720863545457369091[111] = 0;
   out_7720863545457369091[112] = 0;
   out_7720863545457369091[113] = 0;
   out_7720863545457369091[114] = 1;
   out_7720863545457369091[115] = 0;
   out_7720863545457369091[116] = 0;
   out_7720863545457369091[117] = 0;
   out_7720863545457369091[118] = 0;
   out_7720863545457369091[119] = 0;
   out_7720863545457369091[120] = 0;
   out_7720863545457369091[121] = 0;
   out_7720863545457369091[122] = 0;
   out_7720863545457369091[123] = 0;
   out_7720863545457369091[124] = 0;
   out_7720863545457369091[125] = 0;
   out_7720863545457369091[126] = 0;
   out_7720863545457369091[127] = 0;
   out_7720863545457369091[128] = 0;
   out_7720863545457369091[129] = 0;
   out_7720863545457369091[130] = 0;
   out_7720863545457369091[131] = 0;
   out_7720863545457369091[132] = 0;
   out_7720863545457369091[133] = 1;
   out_7720863545457369091[134] = 0;
   out_7720863545457369091[135] = 0;
   out_7720863545457369091[136] = 0;
   out_7720863545457369091[137] = 0;
   out_7720863545457369091[138] = 0;
   out_7720863545457369091[139] = 0;
   out_7720863545457369091[140] = 0;
   out_7720863545457369091[141] = 0;
   out_7720863545457369091[142] = 0;
   out_7720863545457369091[143] = 0;
   out_7720863545457369091[144] = 0;
   out_7720863545457369091[145] = 0;
   out_7720863545457369091[146] = 0;
   out_7720863545457369091[147] = 0;
   out_7720863545457369091[148] = 0;
   out_7720863545457369091[149] = 0;
   out_7720863545457369091[150] = 0;
   out_7720863545457369091[151] = 0;
   out_7720863545457369091[152] = 1;
   out_7720863545457369091[153] = 0;
   out_7720863545457369091[154] = 0;
   out_7720863545457369091[155] = 0;
   out_7720863545457369091[156] = 0;
   out_7720863545457369091[157] = 0;
   out_7720863545457369091[158] = 0;
   out_7720863545457369091[159] = 0;
   out_7720863545457369091[160] = 0;
   out_7720863545457369091[161] = 0;
   out_7720863545457369091[162] = 0;
   out_7720863545457369091[163] = 0;
   out_7720863545457369091[164] = 0;
   out_7720863545457369091[165] = 0;
   out_7720863545457369091[166] = 0;
   out_7720863545457369091[167] = 0;
   out_7720863545457369091[168] = 0;
   out_7720863545457369091[169] = 0;
   out_7720863545457369091[170] = 0;
   out_7720863545457369091[171] = 1;
   out_7720863545457369091[172] = 0;
   out_7720863545457369091[173] = 0;
   out_7720863545457369091[174] = 0;
   out_7720863545457369091[175] = 0;
   out_7720863545457369091[176] = 0;
   out_7720863545457369091[177] = 0;
   out_7720863545457369091[178] = 0;
   out_7720863545457369091[179] = 0;
   out_7720863545457369091[180] = 0;
   out_7720863545457369091[181] = 0;
   out_7720863545457369091[182] = 0;
   out_7720863545457369091[183] = 0;
   out_7720863545457369091[184] = 0;
   out_7720863545457369091[185] = 0;
   out_7720863545457369091[186] = 0;
   out_7720863545457369091[187] = 0;
   out_7720863545457369091[188] = 0;
   out_7720863545457369091[189] = 0;
   out_7720863545457369091[190] = 1;
   out_7720863545457369091[191] = 0;
   out_7720863545457369091[192] = 0;
   out_7720863545457369091[193] = 0;
   out_7720863545457369091[194] = 0;
   out_7720863545457369091[195] = 0;
   out_7720863545457369091[196] = 0;
   out_7720863545457369091[197] = 0;
   out_7720863545457369091[198] = 0;
   out_7720863545457369091[199] = 0;
   out_7720863545457369091[200] = 0;
   out_7720863545457369091[201] = 0;
   out_7720863545457369091[202] = 0;
   out_7720863545457369091[203] = 0;
   out_7720863545457369091[204] = 0;
   out_7720863545457369091[205] = 0;
   out_7720863545457369091[206] = 0;
   out_7720863545457369091[207] = 0;
   out_7720863545457369091[208] = 0;
   out_7720863545457369091[209] = 1;
   out_7720863545457369091[210] = 0;
   out_7720863545457369091[211] = 0;
   out_7720863545457369091[212] = 0;
   out_7720863545457369091[213] = 0;
   out_7720863545457369091[214] = 0;
   out_7720863545457369091[215] = 0;
   out_7720863545457369091[216] = 0;
   out_7720863545457369091[217] = 0;
   out_7720863545457369091[218] = 0;
   out_7720863545457369091[219] = 0;
   out_7720863545457369091[220] = 0;
   out_7720863545457369091[221] = 0;
   out_7720863545457369091[222] = 0;
   out_7720863545457369091[223] = 0;
   out_7720863545457369091[224] = 0;
   out_7720863545457369091[225] = 0;
   out_7720863545457369091[226] = 0;
   out_7720863545457369091[227] = 0;
   out_7720863545457369091[228] = 1;
   out_7720863545457369091[229] = 0;
   out_7720863545457369091[230] = 0;
   out_7720863545457369091[231] = 0;
   out_7720863545457369091[232] = 0;
   out_7720863545457369091[233] = 0;
   out_7720863545457369091[234] = 0;
   out_7720863545457369091[235] = 0;
   out_7720863545457369091[236] = 0;
   out_7720863545457369091[237] = 0;
   out_7720863545457369091[238] = 0;
   out_7720863545457369091[239] = 0;
   out_7720863545457369091[240] = 0;
   out_7720863545457369091[241] = 0;
   out_7720863545457369091[242] = 0;
   out_7720863545457369091[243] = 0;
   out_7720863545457369091[244] = 0;
   out_7720863545457369091[245] = 0;
   out_7720863545457369091[246] = 0;
   out_7720863545457369091[247] = 1;
   out_7720863545457369091[248] = 0;
   out_7720863545457369091[249] = 0;
   out_7720863545457369091[250] = 0;
   out_7720863545457369091[251] = 0;
   out_7720863545457369091[252] = 0;
   out_7720863545457369091[253] = 0;
   out_7720863545457369091[254] = 0;
   out_7720863545457369091[255] = 0;
   out_7720863545457369091[256] = 0;
   out_7720863545457369091[257] = 0;
   out_7720863545457369091[258] = 0;
   out_7720863545457369091[259] = 0;
   out_7720863545457369091[260] = 0;
   out_7720863545457369091[261] = 0;
   out_7720863545457369091[262] = 0;
   out_7720863545457369091[263] = 0;
   out_7720863545457369091[264] = 0;
   out_7720863545457369091[265] = 0;
   out_7720863545457369091[266] = 1;
   out_7720863545457369091[267] = 0;
   out_7720863545457369091[268] = 0;
   out_7720863545457369091[269] = 0;
   out_7720863545457369091[270] = 0;
   out_7720863545457369091[271] = 0;
   out_7720863545457369091[272] = 0;
   out_7720863545457369091[273] = 0;
   out_7720863545457369091[274] = 0;
   out_7720863545457369091[275] = 0;
   out_7720863545457369091[276] = 0;
   out_7720863545457369091[277] = 0;
   out_7720863545457369091[278] = 0;
   out_7720863545457369091[279] = 0;
   out_7720863545457369091[280] = 0;
   out_7720863545457369091[281] = 0;
   out_7720863545457369091[282] = 0;
   out_7720863545457369091[283] = 0;
   out_7720863545457369091[284] = 0;
   out_7720863545457369091[285] = 1;
   out_7720863545457369091[286] = 0;
   out_7720863545457369091[287] = 0;
   out_7720863545457369091[288] = 0;
   out_7720863545457369091[289] = 0;
   out_7720863545457369091[290] = 0;
   out_7720863545457369091[291] = 0;
   out_7720863545457369091[292] = 0;
   out_7720863545457369091[293] = 0;
   out_7720863545457369091[294] = 0;
   out_7720863545457369091[295] = 0;
   out_7720863545457369091[296] = 0;
   out_7720863545457369091[297] = 0;
   out_7720863545457369091[298] = 0;
   out_7720863545457369091[299] = 0;
   out_7720863545457369091[300] = 0;
   out_7720863545457369091[301] = 0;
   out_7720863545457369091[302] = 0;
   out_7720863545457369091[303] = 0;
   out_7720863545457369091[304] = 1;
   out_7720863545457369091[305] = 0;
   out_7720863545457369091[306] = 0;
   out_7720863545457369091[307] = 0;
   out_7720863545457369091[308] = 0;
   out_7720863545457369091[309] = 0;
   out_7720863545457369091[310] = 0;
   out_7720863545457369091[311] = 0;
   out_7720863545457369091[312] = 0;
   out_7720863545457369091[313] = 0;
   out_7720863545457369091[314] = 0;
   out_7720863545457369091[315] = 0;
   out_7720863545457369091[316] = 0;
   out_7720863545457369091[317] = 0;
   out_7720863545457369091[318] = 0;
   out_7720863545457369091[319] = 0;
   out_7720863545457369091[320] = 0;
   out_7720863545457369091[321] = 0;
   out_7720863545457369091[322] = 0;
   out_7720863545457369091[323] = 1;
}
void h_4(double *state, double *unused, double *out_3618725455302009154) {
   out_3618725455302009154[0] = state[6] + state[9];
   out_3618725455302009154[1] = state[7] + state[10];
   out_3618725455302009154[2] = state[8] + state[11];
}
void H_4(double *state, double *unused, double *out_4067297514675495019) {
   out_4067297514675495019[0] = 0;
   out_4067297514675495019[1] = 0;
   out_4067297514675495019[2] = 0;
   out_4067297514675495019[3] = 0;
   out_4067297514675495019[4] = 0;
   out_4067297514675495019[5] = 0;
   out_4067297514675495019[6] = 1;
   out_4067297514675495019[7] = 0;
   out_4067297514675495019[8] = 0;
   out_4067297514675495019[9] = 1;
   out_4067297514675495019[10] = 0;
   out_4067297514675495019[11] = 0;
   out_4067297514675495019[12] = 0;
   out_4067297514675495019[13] = 0;
   out_4067297514675495019[14] = 0;
   out_4067297514675495019[15] = 0;
   out_4067297514675495019[16] = 0;
   out_4067297514675495019[17] = 0;
   out_4067297514675495019[18] = 0;
   out_4067297514675495019[19] = 0;
   out_4067297514675495019[20] = 0;
   out_4067297514675495019[21] = 0;
   out_4067297514675495019[22] = 0;
   out_4067297514675495019[23] = 0;
   out_4067297514675495019[24] = 0;
   out_4067297514675495019[25] = 1;
   out_4067297514675495019[26] = 0;
   out_4067297514675495019[27] = 0;
   out_4067297514675495019[28] = 1;
   out_4067297514675495019[29] = 0;
   out_4067297514675495019[30] = 0;
   out_4067297514675495019[31] = 0;
   out_4067297514675495019[32] = 0;
   out_4067297514675495019[33] = 0;
   out_4067297514675495019[34] = 0;
   out_4067297514675495019[35] = 0;
   out_4067297514675495019[36] = 0;
   out_4067297514675495019[37] = 0;
   out_4067297514675495019[38] = 0;
   out_4067297514675495019[39] = 0;
   out_4067297514675495019[40] = 0;
   out_4067297514675495019[41] = 0;
   out_4067297514675495019[42] = 0;
   out_4067297514675495019[43] = 0;
   out_4067297514675495019[44] = 1;
   out_4067297514675495019[45] = 0;
   out_4067297514675495019[46] = 0;
   out_4067297514675495019[47] = 1;
   out_4067297514675495019[48] = 0;
   out_4067297514675495019[49] = 0;
   out_4067297514675495019[50] = 0;
   out_4067297514675495019[51] = 0;
   out_4067297514675495019[52] = 0;
   out_4067297514675495019[53] = 0;
}
void h_10(double *state, double *unused, double *out_8824774256511149565) {
   out_8824774256511149565[0] = 9.8100000000000005*sin(state[1]) - state[4]*state[8] + state[5]*state[7] + state[12] + state[15];
   out_8824774256511149565[1] = -9.8100000000000005*sin(state[0])*cos(state[1]) + state[3]*state[8] - state[5]*state[6] + state[13] + state[16];
   out_8824774256511149565[2] = -9.8100000000000005*cos(state[0])*cos(state[1]) - state[3]*state[7] + state[4]*state[6] + state[14] + state[17];
}
void H_10(double *state, double *unused, double *out_8138213037672879219) {
   out_8138213037672879219[0] = 0;
   out_8138213037672879219[1] = 9.8100000000000005*cos(state[1]);
   out_8138213037672879219[2] = 0;
   out_8138213037672879219[3] = 0;
   out_8138213037672879219[4] = -state[8];
   out_8138213037672879219[5] = state[7];
   out_8138213037672879219[6] = 0;
   out_8138213037672879219[7] = state[5];
   out_8138213037672879219[8] = -state[4];
   out_8138213037672879219[9] = 0;
   out_8138213037672879219[10] = 0;
   out_8138213037672879219[11] = 0;
   out_8138213037672879219[12] = 1;
   out_8138213037672879219[13] = 0;
   out_8138213037672879219[14] = 0;
   out_8138213037672879219[15] = 1;
   out_8138213037672879219[16] = 0;
   out_8138213037672879219[17] = 0;
   out_8138213037672879219[18] = -9.8100000000000005*cos(state[0])*cos(state[1]);
   out_8138213037672879219[19] = 9.8100000000000005*sin(state[0])*sin(state[1]);
   out_8138213037672879219[20] = 0;
   out_8138213037672879219[21] = state[8];
   out_8138213037672879219[22] = 0;
   out_8138213037672879219[23] = -state[6];
   out_8138213037672879219[24] = -state[5];
   out_8138213037672879219[25] = 0;
   out_8138213037672879219[26] = state[3];
   out_8138213037672879219[27] = 0;
   out_8138213037672879219[28] = 0;
   out_8138213037672879219[29] = 0;
   out_8138213037672879219[30] = 0;
   out_8138213037672879219[31] = 1;
   out_8138213037672879219[32] = 0;
   out_8138213037672879219[33] = 0;
   out_8138213037672879219[34] = 1;
   out_8138213037672879219[35] = 0;
   out_8138213037672879219[36] = 9.8100000000000005*sin(state[0])*cos(state[1]);
   out_8138213037672879219[37] = 9.8100000000000005*sin(state[1])*cos(state[0]);
   out_8138213037672879219[38] = 0;
   out_8138213037672879219[39] = -state[7];
   out_8138213037672879219[40] = state[6];
   out_8138213037672879219[41] = 0;
   out_8138213037672879219[42] = state[4];
   out_8138213037672879219[43] = -state[3];
   out_8138213037672879219[44] = 0;
   out_8138213037672879219[45] = 0;
   out_8138213037672879219[46] = 0;
   out_8138213037672879219[47] = 0;
   out_8138213037672879219[48] = 0;
   out_8138213037672879219[49] = 0;
   out_8138213037672879219[50] = 1;
   out_8138213037672879219[51] = 0;
   out_8138213037672879219[52] = 0;
   out_8138213037672879219[53] = 1;
}
void h_13(double *state, double *unused, double *out_401660460692188095) {
   out_401660460692188095[0] = state[3];
   out_401660460692188095[1] = state[4];
   out_401660460692188095[2] = state[5];
}
void H_13(double *state, double *unused, double *out_7279571340007827820) {
   out_7279571340007827820[0] = 0;
   out_7279571340007827820[1] = 0;
   out_7279571340007827820[2] = 0;
   out_7279571340007827820[3] = 1;
   out_7279571340007827820[4] = 0;
   out_7279571340007827820[5] = 0;
   out_7279571340007827820[6] = 0;
   out_7279571340007827820[7] = 0;
   out_7279571340007827820[8] = 0;
   out_7279571340007827820[9] = 0;
   out_7279571340007827820[10] = 0;
   out_7279571340007827820[11] = 0;
   out_7279571340007827820[12] = 0;
   out_7279571340007827820[13] = 0;
   out_7279571340007827820[14] = 0;
   out_7279571340007827820[15] = 0;
   out_7279571340007827820[16] = 0;
   out_7279571340007827820[17] = 0;
   out_7279571340007827820[18] = 0;
   out_7279571340007827820[19] = 0;
   out_7279571340007827820[20] = 0;
   out_7279571340007827820[21] = 0;
   out_7279571340007827820[22] = 1;
   out_7279571340007827820[23] = 0;
   out_7279571340007827820[24] = 0;
   out_7279571340007827820[25] = 0;
   out_7279571340007827820[26] = 0;
   out_7279571340007827820[27] = 0;
   out_7279571340007827820[28] = 0;
   out_7279571340007827820[29] = 0;
   out_7279571340007827820[30] = 0;
   out_7279571340007827820[31] = 0;
   out_7279571340007827820[32] = 0;
   out_7279571340007827820[33] = 0;
   out_7279571340007827820[34] = 0;
   out_7279571340007827820[35] = 0;
   out_7279571340007827820[36] = 0;
   out_7279571340007827820[37] = 0;
   out_7279571340007827820[38] = 0;
   out_7279571340007827820[39] = 0;
   out_7279571340007827820[40] = 0;
   out_7279571340007827820[41] = 1;
   out_7279571340007827820[42] = 0;
   out_7279571340007827820[43] = 0;
   out_7279571340007827820[44] = 0;
   out_7279571340007827820[45] = 0;
   out_7279571340007827820[46] = 0;
   out_7279571340007827820[47] = 0;
   out_7279571340007827820[48] = 0;
   out_7279571340007827820[49] = 0;
   out_7279571340007827820[50] = 0;
   out_7279571340007827820[51] = 0;
   out_7279571340007827820[52] = 0;
   out_7279571340007827820[53] = 0;
}
void h_14(double *state, double *unused, double *out_1422683178583968447) {
   out_1422683178583968447[0] = state[6];
   out_1422683178583968447[1] = state[7];
   out_1422683178583968447[2] = state[8];
}
void H_14(double *state, double *unused, double *out_8030538371014979548) {
   out_8030538371014979548[0] = 0;
   out_8030538371014979548[1] = 0;
   out_8030538371014979548[2] = 0;
   out_8030538371014979548[3] = 0;
   out_8030538371014979548[4] = 0;
   out_8030538371014979548[5] = 0;
   out_8030538371014979548[6] = 1;
   out_8030538371014979548[7] = 0;
   out_8030538371014979548[8] = 0;
   out_8030538371014979548[9] = 0;
   out_8030538371014979548[10] = 0;
   out_8030538371014979548[11] = 0;
   out_8030538371014979548[12] = 0;
   out_8030538371014979548[13] = 0;
   out_8030538371014979548[14] = 0;
   out_8030538371014979548[15] = 0;
   out_8030538371014979548[16] = 0;
   out_8030538371014979548[17] = 0;
   out_8030538371014979548[18] = 0;
   out_8030538371014979548[19] = 0;
   out_8030538371014979548[20] = 0;
   out_8030538371014979548[21] = 0;
   out_8030538371014979548[22] = 0;
   out_8030538371014979548[23] = 0;
   out_8030538371014979548[24] = 0;
   out_8030538371014979548[25] = 1;
   out_8030538371014979548[26] = 0;
   out_8030538371014979548[27] = 0;
   out_8030538371014979548[28] = 0;
   out_8030538371014979548[29] = 0;
   out_8030538371014979548[30] = 0;
   out_8030538371014979548[31] = 0;
   out_8030538371014979548[32] = 0;
   out_8030538371014979548[33] = 0;
   out_8030538371014979548[34] = 0;
   out_8030538371014979548[35] = 0;
   out_8030538371014979548[36] = 0;
   out_8030538371014979548[37] = 0;
   out_8030538371014979548[38] = 0;
   out_8030538371014979548[39] = 0;
   out_8030538371014979548[40] = 0;
   out_8030538371014979548[41] = 0;
   out_8030538371014979548[42] = 0;
   out_8030538371014979548[43] = 0;
   out_8030538371014979548[44] = 1;
   out_8030538371014979548[45] = 0;
   out_8030538371014979548[46] = 0;
   out_8030538371014979548[47] = 0;
   out_8030538371014979548[48] = 0;
   out_8030538371014979548[49] = 0;
   out_8030538371014979548[50] = 0;
   out_8030538371014979548[51] = 0;
   out_8030538371014979548[52] = 0;
   out_8030538371014979548[53] = 0;
}
#include <eigen3/Eigen/Dense>
#include <iostream>

typedef Eigen::Matrix<double, DIM, DIM, Eigen::RowMajor> DDM;
typedef Eigen::Matrix<double, EDIM, EDIM, Eigen::RowMajor> EEM;
typedef Eigen::Matrix<double, DIM, EDIM, Eigen::RowMajor> DEM;

void predict(double *in_x, double *in_P, double *in_Q, double dt) {
  typedef Eigen::Matrix<double, MEDIM, MEDIM, Eigen::RowMajor> RRM;

  double nx[DIM] = {0};
  double in_F[EDIM*EDIM] = {0};

  // functions from sympy
  f_fun(in_x, dt, nx);
  F_fun(in_x, dt, in_F);


  EEM F(in_F);
  EEM P(in_P);
  EEM Q(in_Q);

  RRM F_main = F.topLeftCorner(MEDIM, MEDIM);
  P.topLeftCorner(MEDIM, MEDIM) = (F_main * P.topLeftCorner(MEDIM, MEDIM)) * F_main.transpose();
  P.topRightCorner(MEDIM, EDIM - MEDIM) = F_main * P.topRightCorner(MEDIM, EDIM - MEDIM);
  P.bottomLeftCorner(EDIM - MEDIM, MEDIM) = P.bottomLeftCorner(EDIM - MEDIM, MEDIM) * F_main.transpose();

  P = P + dt*Q;

  // copy out state
  memcpy(in_x, nx, DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
}

// note: extra_args dim only correct when null space projecting
// otherwise 1
template <int ZDIM, int EADIM, bool MAHA_TEST>
void update(double *in_x, double *in_P, Hfun h_fun, Hfun H_fun, Hfun Hea_fun, double *in_z, double *in_R, double *in_ea, double MAHA_THRESHOLD) {
  typedef Eigen::Matrix<double, ZDIM, ZDIM, Eigen::RowMajor> ZZM;
  typedef Eigen::Matrix<double, ZDIM, DIM, Eigen::RowMajor> ZDM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, EDIM, Eigen::RowMajor> XEM;
  //typedef Eigen::Matrix<double, EDIM, ZDIM, Eigen::RowMajor> EZM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, 1> X1M;
  typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> XXM;

  double in_hx[ZDIM] = {0};
  double in_H[ZDIM * DIM] = {0};
  double in_H_mod[EDIM * DIM] = {0};
  double delta_x[EDIM] = {0};
  double x_new[DIM] = {0};


  // state x, P
  Eigen::Matrix<double, ZDIM, 1> z(in_z);
  EEM P(in_P);
  ZZM pre_R(in_R);

  // functions from sympy
  h_fun(in_x, in_ea, in_hx);
  H_fun(in_x, in_ea, in_H);
  ZDM pre_H(in_H);

  // get y (y = z - hx)
  Eigen::Matrix<double, ZDIM, 1> pre_y(in_hx); pre_y = z - pre_y;
  X1M y; XXM H; XXM R;
  if (Hea_fun){
    typedef Eigen::Matrix<double, ZDIM, EADIM, Eigen::RowMajor> ZAM;
    double in_Hea[ZDIM * EADIM] = {0};
    Hea_fun(in_x, in_ea, in_Hea);
    ZAM Hea(in_Hea);
    XXM A = Hea.transpose().fullPivLu().kernel();


    y = A.transpose() * pre_y;
    H = A.transpose() * pre_H;
    R = A.transpose() * pre_R * A;
  } else {
    y = pre_y;
    H = pre_H;
    R = pre_R;
  }
  // get modified H
  H_mod_fun(in_x, in_H_mod);
  DEM H_mod(in_H_mod);
  XEM H_err = H * H_mod;

  // Do mahalobis distance test
  if (MAHA_TEST){
    XXM a = (H_err * P * H_err.transpose() + R).inverse();
    double maha_dist = y.transpose() * a * y;
    if (maha_dist > MAHA_THRESHOLD){
      R = 1.0e16 * R;
    }
  }

  // Outlier resilient weighting
  double weight = 1;//(1.5)/(1 + y.squaredNorm()/R.sum());

  // kalman gains and I_KH
  XXM S = ((H_err * P) * H_err.transpose()) + R/weight;
  XEM KT = S.fullPivLu().solve(H_err * P.transpose());
  //EZM K = KT.transpose(); TODO: WHY DOES THIS NOT COMPILE?
  //EZM K = S.fullPivLu().solve(H_err * P.transpose()).transpose();
  //std::cout << "Here is the matrix rot:\n" << K << std::endl;
  EEM I_KH = Eigen::Matrix<double, EDIM, EDIM>::Identity() - (KT.transpose() * H_err);

  // update state by injecting dx
  Eigen::Matrix<double, EDIM, 1> dx(delta_x);
  dx  = (KT.transpose() * y);
  memcpy(delta_x, dx.data(), EDIM * sizeof(double));
  err_fun(in_x, delta_x, x_new);
  Eigen::Matrix<double, DIM, 1> x(x_new);

  // update cov
  P = ((I_KH * P) * I_KH.transpose()) + ((KT.transpose() * R) * KT);

  // copy out state
  memcpy(in_x, x.data(), DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
  memcpy(in_z, y.data(), y.rows() * sizeof(double));
}




}
extern "C" {

void pose_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_4, H_4, NULL, in_z, in_R, in_ea, MAHA_THRESH_4);
}
void pose_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_10, H_10, NULL, in_z, in_R, in_ea, MAHA_THRESH_10);
}
void pose_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_13, H_13, NULL, in_z, in_R, in_ea, MAHA_THRESH_13);
}
void pose_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_14, H_14, NULL, in_z, in_R, in_ea, MAHA_THRESH_14);
}
void pose_err_fun(double *nom_x, double *delta_x, double *out_565953791747580906) {
  err_fun(nom_x, delta_x, out_565953791747580906);
}
void pose_inv_err_fun(double *nom_x, double *true_x, double *out_5141089249990947268) {
  inv_err_fun(nom_x, true_x, out_5141089249990947268);
}
void pose_H_mod_fun(double *state, double *out_8531364752282097876) {
  H_mod_fun(state, out_8531364752282097876);
}
void pose_f_fun(double *state, double dt, double *out_8938703759442158393) {
  f_fun(state,  dt, out_8938703759442158393);
}
void pose_F_fun(double *state, double dt, double *out_7720863545457369091) {
  F_fun(state,  dt, out_7720863545457369091);
}
void pose_h_4(double *state, double *unused, double *out_3618725455302009154) {
  h_4(state, unused, out_3618725455302009154);
}
void pose_H_4(double *state, double *unused, double *out_4067297514675495019) {
  H_4(state, unused, out_4067297514675495019);
}
void pose_h_10(double *state, double *unused, double *out_8824774256511149565) {
  h_10(state, unused, out_8824774256511149565);
}
void pose_H_10(double *state, double *unused, double *out_8138213037672879219) {
  H_10(state, unused, out_8138213037672879219);
}
void pose_h_13(double *state, double *unused, double *out_401660460692188095) {
  h_13(state, unused, out_401660460692188095);
}
void pose_H_13(double *state, double *unused, double *out_7279571340007827820) {
  H_13(state, unused, out_7279571340007827820);
}
void pose_h_14(double *state, double *unused, double *out_1422683178583968447) {
  h_14(state, unused, out_1422683178583968447);
}
void pose_H_14(double *state, double *unused, double *out_8030538371014979548) {
  H_14(state, unused, out_8030538371014979548);
}
void pose_predict(double *in_x, double *in_P, double *in_Q, double dt) {
  predict(in_x, in_P, in_Q, dt);
}
}

const EKF pose = {
  .name = "pose",
  .kinds = { 4, 10, 13, 14 },
  .feature_kinds = {  },
  .f_fun = pose_f_fun,
  .F_fun = pose_F_fun,
  .err_fun = pose_err_fun,
  .inv_err_fun = pose_inv_err_fun,
  .H_mod_fun = pose_H_mod_fun,
  .predict = pose_predict,
  .hs = {
    { 4, pose_h_4 },
    { 10, pose_h_10 },
    { 13, pose_h_13 },
    { 14, pose_h_14 },
  },
  .Hs = {
    { 4, pose_H_4 },
    { 10, pose_H_10 },
    { 13, pose_H_13 },
    { 14, pose_H_14 },
  },
  .updates = {
    { 4, pose_update_4 },
    { 10, pose_update_10 },
    { 13, pose_update_13 },
    { 14, pose_update_14 },
  },
  .Hes = {
  },
  .sets = {
  },
  .extra_routines = {
  },
};

ekf_lib_init(pose)
