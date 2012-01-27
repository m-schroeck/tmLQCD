#pragma once

#include <smearing/ape.h>
#include <smearing/utils.h>

struct hyp_parameters
{
  double alpha[3];
  int    iterations;
};

/* All defined in terms of arrays of tuples -- needed to allow for g_gauge_field as input */

void hyp_staples_exclude_none(gauge_field_t buff_out, gauge_field_array_t buff_in); /* 12 components in, 4 components out */
void hyp_staples_exclude_one (gauge_field_array_t buff_out, gauge_field_array_t buff_in);  /* 12 components in, 12 components out */
void hyp_staples_exclude_two (gauge_field_array_t buff_out, gauge_field_t buff_in);  /*  4 components in, 12 components out */

void APE_project_exclude_one (gauge_field_array_t buff_out, double const coeff, gauge_field_array_t staples, gauge_field_t buff_in);
void APE_project_exclude_two (gauge_field_array_t buff_out, double const coeff, gauge_field_array_t staples, gauge_field_t buff_in);

int hyp_smear(gauge_field_t m_field_out, struct hyp_parameters const *params, gauge_field_t m_field_in);  /*  4 components in, 4 components out */
