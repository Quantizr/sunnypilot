#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_err_fun(double *nom_x, double *delta_x, double *out_897898146241201178);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_6525795040583645782);
void car_H_mod_fun(double *state, double *out_5442212724688376850);
void car_f_fun(double *state, double dt, double *out_8387369996841538347);
void car_F_fun(double *state, double dt, double *out_6737129550653399619);
void car_h_25(double *state, double *unused, double *out_1105750886784299517);
void car_H_25(double *state, double *unused, double *out_8965261363333741998);
void car_h_24(double *state, double *unused, double *out_574672407512177041);
void car_H_24(double *state, double *unused, double *out_6739553579354873436);
void car_h_30(double *state, double *unused, double *out_4967434363212801169);
void car_H_30(double *state, double *unused, double *out_2048571021842125243);
void car_h_26(double *state, double *unused, double *out_8148759939640259285);
void car_H_26(double *state, double *unused, double *out_5660735393572941397);
void car_h_27(double *state, double *unused, double *out_873860883132214585);
void car_H_27(double *state, double *unused, double *out_4223334333642550154);
void car_h_29(double *state, double *unused, double *out_5570750247889278686);
void car_H_29(double *state, double *unused, double *out_1538339677527733059);
void car_h_28(double *state, double *unused, double *out_4352046332703110362);
void car_H_28(double *state, double *unused, double *out_7427647996127919855);
void car_h_31(double *state, double *unused, double *out_1054790772674272930);
void car_H_31(double *state, double *unused, double *out_1888586112821924745);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}