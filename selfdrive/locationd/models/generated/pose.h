#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void pose_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_err_fun(double *nom_x, double *delta_x, double *out_565953791747580906);
void pose_inv_err_fun(double *nom_x, double *true_x, double *out_5141089249990947268);
void pose_H_mod_fun(double *state, double *out_8531364752282097876);
void pose_f_fun(double *state, double dt, double *out_8938703759442158393);
void pose_F_fun(double *state, double dt, double *out_7720863545457369091);
void pose_h_4(double *state, double *unused, double *out_3618725455302009154);
void pose_H_4(double *state, double *unused, double *out_4067297514675495019);
void pose_h_10(double *state, double *unused, double *out_8824774256511149565);
void pose_H_10(double *state, double *unused, double *out_8138213037672879219);
void pose_h_13(double *state, double *unused, double *out_401660460692188095);
void pose_H_13(double *state, double *unused, double *out_7279571340007827820);
void pose_h_14(double *state, double *unused, double *out_1422683178583968447);
void pose_H_14(double *state, double *unused, double *out_8030538371014979548);
void pose_predict(double *in_x, double *in_P, double *in_Q, double dt);
}