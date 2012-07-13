#ifndef _BGQ_H
#define _BGQ_H

//#define regtype vector4double

inline void vec_load2(vector4double * r, su3_vector * const phi) {
#pragma disjoint(*r,*phi)
  r[0] = vec_ld2(0, (double*) &phi->c0); 
  r[1] = vec_ld2(0, (double*) &phi->c1);
  r[2] = vec_ld2(0, (double*) &phi->c2);
  return;
}

inline void vec_store2(su3_vector * const phi, vector4double * r) {
#pragma disjoint(*r,*phi)
  vec_st2(r[0], 0, (double*) &phi->c0);
  vec_st2(r[1], 0, (double*) &phi->c1);
  vec_st2(r[2], 0, (double*) &phi->c2);
  return;
}

// r = r + s
inline void vec_add2(vector4double * restrict r, vector4double * restrict s) {
#pragma disjoint(*s, *r)
  r[0] = vec_add(r[0], s[0]);
  r[1] = vec_add(r[1], s[1]);
  r[2] = vec_add(r[2], s[2]);
  return;
}

// r = r + s
inline void vec_add_double2(vector4double * restrict r, vector4double * restrict s) {
#pragma disjoint(*s, *r)
  r[0] = vec_add(r[0], s[0]);
  r[1] = vec_add(r[1], s[1]);
  r[2] = vec_add(r[2], s[2]);
  r[3] = vec_add(r[3], s[3]);
  r[4] = vec_add(r[4], s[4]);
  r[5] = vec_add(r[5], s[5]);

  return;
}

// r = r - s
inline void vec_sub2(vector4double * restrict r, vector4double * restrict s) {
#pragma disjoint(*s, *r)
  r[0] = vec_sub(r[0], s[0]);
  r[1] = vec_sub(r[1], s[1]);
  r[2] = vec_sub(r[2], s[2]);
  return;
}

// r = r - s
inline void vec_sub_double2(vector4double * restrict r, vector4double * restrict s) {
#pragma disjoint(*s, *r)
  r[0] = vec_sub(r[0], s[0]);
  r[1] = vec_sub(r[1], s[1]);
  r[2] = vec_sub(r[2], s[2]);
  r[3] = vec_sub(r[3], s[3]);
  r[4] = vec_sub(r[4], s[4]);
  r[5] = vec_sub(r[5], s[5]);

  return;
}

// r = r + i*s
// tmp, temporary memory
inline void vec_i_mul_add2(vector4double * restrict r, vector4double * restrict s, 
			   vector4double * restrict tmp) {
#pragma disjoint(*s, *r)
#pragma disjoint(*s, *tmp)
#pragma disjoint(*r, *tmp)
  tmp[0] = vec_splats(1.);
  r[0] = vec_xxnpmadd(s[0], tmp[0], r[0]);
  r[1] = vec_xxnpmadd(s[1], tmp[0], r[1]);
  r[2] = vec_xxnpmadd(s[2], tmp[0], r[2]);
  return;
}

// r = r + i*s
// tmp, temporary memory
inline void vec_i_mul_add_double2(vector4double * restrict r, vector4double * restrict s, 
				  vector4double * restrict tmp) {
#pragma disjoint(*s, *r)
#pragma disjoint(*s, *tmp)
#pragma disjoint(*r, *tmp)
  tmp[0] = vec_splats(1.);
  r[0] = vec_xxnpmadd(s[0], tmp[0], r[0]);
  r[1] = vec_xxnpmadd(s[1], tmp[0], r[1]);
  r[2] = vec_xxnpmadd(s[2], tmp[0], r[2]);
  r[3] = vec_xxnpmadd(s[3], tmp[0], r[3]);
  r[4] = vec_xxnpmadd(s[4], tmp[0], r[4]);
  r[5] = vec_xxnpmadd(s[5], tmp[0], r[5]);
  return;
}

// r = r - i*s
// tmp, temporary memory
inline void vec_i_mul_sub2(vector4double * restrict r, vector4double * restrict s, 
			   vector4double * restrict tmp) {
#pragma disjoint(*s, *r)
#pragma disjoint(*s, *tmp)
#pragma disjoint(*r, *tmp)
  tmp[0] = vec_splats(-1.);
  r[0] = vec_xxnpmadd(s[0], tmp[0], r[0]);
  r[1] = vec_xxnpmadd(s[1], tmp[0], r[1]);
  r[2] = vec_xxnpmadd(s[2], tmp[0], r[2]);
  return;
}

// r = r - i*s
// tmp, temporary memory
inline void vec_i_mul_sub_double2(vector4double * restrict r, vector4double * restrict s, 
				  vector4double * restrict tmp) {
#pragma disjoint(*s, *r)
#pragma disjoint(*s, *tmp)
#pragma disjoint(*r, *tmp)
  tmp[0] = vec_splats(-1.);
  r[0] = vec_xxnpmadd(s[0], tmp[0], r[0]);
  r[1] = vec_xxnpmadd(s[1], tmp[0], r[1]);
  r[2] = vec_xxnpmadd(s[2], tmp[0], r[2]);
  r[3] = vec_xxnpmadd(s[3], tmp[0], r[3]);
  r[4] = vec_xxnpmadd(s[4], tmp[0], r[4]);
  r[5] = vec_xxnpmadd(s[5], tmp[0], r[5]);
  return;
}



inline void vec_cmplx_mul_double2(vector4double * restrict rs, vector4double * restrict r,
				  vector4double * tmp, complex double * c) {
#pragma disjoint(*rs, *r)
#pragma disjoint(*r, *tmp)
#pragma disjoint(*tmp, rs)
  __alignx(32, c);
  tmp[0] = vec_ld2(0, (double*) c);
  rs[0] = vec_xmul(r[0], tmp[0]);
  rs[1] = vec_xmul(r[1], tmp[0]);
  rs[2] = vec_xmul(r[2], tmp[0]);
  rs[3] = vec_xmul(r[3], tmp[0]);
  rs[4] = vec_xmul(r[4], tmp[0]);
  rs[5] = vec_xmul(r[5], tmp[0]);
  rs[0] = vec_xxnpmadd(tmp[0], r[0], rs[0]);
  rs[1] = vec_xxnpmadd(tmp[0], r[1], rs[1]);
  rs[2] = vec_xxnpmadd(tmp[0], r[2], rs[2]);
  rs[3] = vec_xxnpmadd(tmp[0], r[3], rs[3]);
  rs[4] = vec_xxnpmadd(tmp[0], r[4], rs[4]);
  rs[5] = vec_xxnpmadd(tmp[0], r[5], rs[5]);
  return;
}

inline void vec_cmplxcg_mul_double2(vector4double * restrict rs, vector4double * restrict r,
				    vector4double * tmp, complex double * c) {
#pragma disjoint(*rs, *r)
#pragma disjoint(*r, *tmp)
#pragma disjoint(*tmp, rs)
  __alignx(32, c);
  tmp[0] = vec_ld2(0, (double*) c);
  rs[0] = vec_xmul(tmp[0], r[0]);
  rs[1] = vec_xmul(tmp[0], r[1]);
  rs[2] = vec_xmul(tmp[0], r[2]);
  rs[3] = vec_xmul(tmp[0], r[3]);
  rs[4] = vec_xmul(tmp[0], r[4]);
  rs[5] = vec_xmul(tmp[0], r[5]);
  rs[0] = vec_xxcpnmadd(r[0], tmp[0], rs[0]);
  rs[1] = vec_xxcpnmadd(r[1], tmp[0], rs[1]);
  rs[2] = vec_xxcpnmadd(r[2], tmp[0], rs[2]);
  rs[3] = vec_xxcpnmadd(r[3], tmp[0], rs[3]);
  rs[4] = vec_xxcpnmadd(r[4], tmp[0], rs[4]);
  rs[5] = vec_xxcpnmadd(r[5], tmp[0], rs[5]);
  return;
}


// multiplies one su3 matrix with two su3_vectors
// the first of which stored in r[0-2]
// and the second one in r[3-5]
//
// the resulting two vectors are stored in
// r[6-11]
//
// this routine uses only half of the 4 doubles in vector4double

inline void vec_su3_multiply_double2(su3 * const restrict u, vector4double * restrict U, 
				     vector4double * restrict r) {
#pragma disjoint(*U, *r)
  __alignx(32, u);
  __alignx(32, U);
  __alignx(32, r);

  U[0] = vec_ld2(0, (double*) &u->c00);
  U[3] = vec_ld2(0, (double*) &u->c01);
  U[6] = vec_ld2(0, (double*) &u->c02);
  U[1] = vec_ld2(0, (double*) &u->c10);
  U[4] = vec_ld2(0, (double*) &u->c11);
  U[7] = vec_ld2(0, (double*) &u->c12);
  U[2] = vec_ld2(0, (double*) &u->c20);
  r[6] = vec_xmul(r[0], U[0]);
  r[7] = vec_xmul(r[0], U[1]);
  r[8] = vec_xmul(r[0], U[2]);
  r[9] = vec_xmul(r[3], U[0]);
  r[10] = vec_xmul(r[3], U[1]);
  r[11] = vec_xmul(r[3], U[2]);

  r[6] = vec_xxnpmadd(U[0], r[0], r[6]);
  r[7] = vec_xxnpmadd(U[1], r[0], r[7]);
  r[8] = vec_xxnpmadd(U[2], r[0], r[8]);
  r[9] = vec_xxnpmadd(U[0], r[3], r[9]);
  r[10] = vec_xxnpmadd(U[1], r[3], r[10]);
  r[11] = vec_xxnpmadd(U[2], r[3], r[11]);
  U[5] = vec_ld2(0, (double*) &u->c21);

  r[6] = vec_xmadd(r[1], U[3], r[6]);
  r[7] = vec_xmadd(r[1], U[4], r[7]);
  r[8] = vec_xmadd(r[1], U[5], r[8]);
  r[9] = vec_xmadd(r[4], U[3], r[9]);
  r[10] = vec_xmadd(r[4], U[4], r[10]);
  r[11] = vec_xmadd(r[4], U[5], r[11]);

  r[6] = vec_xxnpmadd(U[3], r[1], r[6]);
  r[7] = vec_xxnpmadd(U[4], r[1], r[7]);
  r[8] = vec_xxnpmadd(U[5], r[1], r[8]);
  r[9] = vec_xxnpmadd(U[3], r[4], r[9]);
  r[10] = vec_xxnpmadd(U[4], r[4], r[10]);
  r[11] = vec_xxnpmadd(U[5], r[4], r[11]);
  U[8] = vec_ld2(0, (double*) &u->c22);

  r[6] = vec_xmadd(r[2], U[6], r[6]);
  r[7] = vec_xmadd(r[2], U[7], r[7]);
  r[8] = vec_xmadd(r[2], U[8], r[8]);
  r[9] = vec_xmadd(r[5], U[6], r[9]);
  r[10] = vec_xmadd(r[5], U[7], r[10]);
  r[11] = vec_xmadd(r[5], U[8], r[11]);

  r[6] = vec_xxnpmadd(U[6], r[2], r[6]);
  r[7] = vec_xxnpmadd(U[7], r[2], r[7]);
  r[8] = vec_xxnpmadd(U[8], r[2], r[8]);
  r[9] = vec_xxnpmadd(U[6], r[5], r[9]);
  r[10] = vec_xxnpmadd(U[7], r[5], r[10]);
  r[11] = vec_xxnpmadd(U[8], r[5], r[11]);
  return;
}

// multiplies the inverse of one su3 matrix with two su3_vectors
// the first of which stored in r[0-2]
// and the second one in r[3-5]
//
// the resulting two vectors are stored in
// r[6-11]
//
// this routine uses only half of the 4 doubles in vector4double

inline void vec_su3_inverse_multiply_double2(su3 * const restrict u, vector4double * restrict U, 
					     vector4double * restrict r) {
#pragma disjoint(*U, *r)
  __alignx(32, u);
  __alignx(32, U);
  __alignx(32, r);

  U[0] = vec_ld2(0, (double*) &u->c00);
  U[1] = vec_ld2(0, (double*) &u->c01);
  U[2] = vec_ld2(0, (double*) &u->c02);

  r[6] = vec_xmul(U[0], r[0]);
  r[7] = vec_xmul(U[1], r[0]);
  r[8] = vec_xmul(U[2], r[0]);
  r[9] = vec_xmul(U[0], r[3]);
  r[10] = vec_xmul(U[1], r[3]);
  r[11] = vec_xmul(U[2], r[3]);

  r[6] = vec_xxcpnmadd(r[0], U[0], r[6]);
  r[7] = vec_xxcpnmadd(r[0], U[1], r[7]);
  r[8] = vec_xxcpnmadd(r[0], U[2], r[8]);
  r[9] = vec_xxcpnmadd(r[3], U[0], r[9]);
  r[10] = vec_xxcpnmadd(r[3], U[1], r[10]);
  r[11] = vec_xxcpnmadd(r[3], U[2], r[11]);

  U[0] = vec_ld2(0, (double*) &u->c10);
  U[1] = vec_ld2(0, (double*) &u->c11);
  U[2] = vec_ld2(0, (double*) &u->c12);

  r[6] = vec_xmadd(U[0], r[1], r[6]);
  r[7] = vec_xmadd(U[1], r[1], r[7]);
  r[8] = vec_xmadd(U[2], r[1], r[8]);
  r[9] = vec_xmadd(U[0], r[4], r[9]);
  r[10] = vec_xmadd(U[1], r[4], r[10]);
  r[11] = vec_xmadd(U[2], r[4], r[11]);
  
  r[6] = vec_xxcpnmadd(r[1], U[0], r[6]);
  r[7] = vec_xxcpnmadd(r[1], U[1], r[7]);
  r[8] = vec_xxcpnmadd(r[1], U[2], r[8]);
  r[9] = vec_xxcpnmadd(r[4], U[0], r[9]);
  r[10] = vec_xxcpnmadd(r[4], U[1], r[10]);
  r[11] = vec_xxcpnmadd(r[4], U[2], r[11]);

  U[0] = vec_ld2(0, (double*) &u->c20);
  U[1] = vec_ld2(0, (double*) &u->c21);
  U[2] = vec_ld2(0, (double*) &u->c22);

  r[6] = vec_xmadd(U[0], r[2], r[6]);
  r[7] = vec_xmadd(U[1], r[2], r[7]);
  r[8] = vec_xmadd(U[2], r[2], r[8]);
  r[9] = vec_xmadd(U[0], r[5], r[9]);
  r[10] = vec_xmadd(U[1], r[5], r[10]);
  r[11] = vec_xmadd(U[2], r[5], r[11]);

  r[6] = vec_xxcpnmadd(r[2], U[0], r[6]);
  r[7] = vec_xxcpnmadd(r[2], U[1], r[7]);
  r[8] = vec_xxcpnmadd(r[2], U[2], r[8]);
  r[9] = vec_xxcpnmadd(r[5], U[0], r[9]);
  r[10] = vec_xxcpnmadd(r[5], U[1], r[10]);
  r[11] = vec_xxcpnmadd(r[5], U[2], r[11]);
  return;
}


// alternative implementation
//
// might not be optimal for pipeline as result is 
// re-used in the next line.
inline void vec_su3_multiply_double2b(su3 * const u, vector4double * U, vector4double * r) {
  __alignx(32, u);
  __alignx(32, U);
  __alignx(32, r);
  U[0] = vec_ld2(0, (double*) &u->c00);
  U[1] = vec_ld2(0, (double*) &u->c01);
  U[2] = vec_ld2(0, (double*) &u->c02);

  r[6] = vec_xmul(r[0], U[0]);
  r[6] = vec_xxnpmadd(U[0], r[0], r[6]);
#pragma unroll(2)
  for(int i = 1; i < 3; i++) {
    r[6] = vec_xmadd(r[i], U[i], r[6]);
    r[6] = vec_xxnpmadd(U[i], r[i], r[6]);
  }
  r[9] = vec_xmul(r[3], U[0]);
  r[9] = vec_xxnpmadd(U[0], r[3], r[9]);
#pragma unroll(2)
  for(int i = 1; i < 3; i++) {
    r[9] = vec_xmadd(r[3+i], U[i], r[9]);
    r[9] = vec_xxnpmadd(U[i], r[3+i], r[9]);
  }

  U[0] = vec_ld2(0, (double*) &u->c10);
  U[1] = vec_ld2(0, (double*) &u->c11);
  U[2] = vec_ld2(0, (double*) &u->c12);

  r[7] = vec_xmul(r[0], U[0]);
  r[7] = vec_xxnpmadd(U[0], r[0], r[7]);
#pragma unroll(2)
  for(int i = 1; i < 3; i++) {
    r[7] = vec_xmadd(r[i], U[i], r[7]);
    r[7] = vec_xxnpmadd(U[i], r[i], r[7]);
  }
  r[10] = vec_xmul(r[3], U[0]);
  r[10] = vec_xxnpmadd(U[0], r[3], r[10]);
#pragma unroll(2)
  for(int i = 1; i < 3; i++) {
    r[10] = vec_xmadd(r[3+i], U[i], r[10]);
    r[10] = vec_xxnpmadd(U[i], r[3+i], r[10]);
  }

  U[0] = vec_ld2(0, (double*) &u->c20);
  U[1] = vec_ld2(0, (double*) &u->c21);
  U[2] = vec_ld2(0, (double*) &u->c22);

  r[8] = vec_xmul(r[0], U[0]);
  r[8] = vec_xxnpmadd(U[0], r[0], r[8]);
#pragma unroll(2)
  for(int i = 1; i < 3; i++) {
    r[8] = vec_xmadd(r[i], U[i], r[8]);
    r[8] = vec_xxnpmadd(U[i], r[i], r[8]);
  }
  r[11] = vec_xmul(r[3], U[0]);
  r[11] = vec_xxnpmadd(U[0], r[3], r[11]);
#pragma unroll(2)
  for(int i = 1; i < 3; i++) {
    r[11] = vec_xmadd(r[3+i], U[i], r[11]);
    r[11] = vec_xxnpmadd(U[i], r[3+i], r[11]);
  }
  return;
}


#endif