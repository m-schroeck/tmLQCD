/***********************************************************************
 *
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008 Carsten Urbach
 *               2015 Mario Schroeck
 *
 * This file is part of tmLQCD.
 *
 * tmLQCD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tmLQCD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tmLQCD.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This is a program to approximate the disconnected diagrams (aka. bubbles).
 * On a given gaugefield configuration, it calculates
 *
 * B(x,t)^{ba_0}_{\beta\alpha_0} \equiv \sum_{x,t} D^{-1}(x,t;x_0,t_0)^{ba_0}_{\beta\alpha_0}\;\delta_{xx_0}\,\delta_{tt_0}
	\quad\forall\; a_0, \alpha_0, x_0, t_0
 *
 * via stochastic approximation and full dilution in color, space and time.
 * Only at the end the bubbles B(x,t) are stored to disk, note that B(x,t) has the
 * same dimensions as a regular point-to-all quark propagator.
 *
 *
 *******************************************************************************/

#include"lime.h"
#ifdef HAVE_CONFIG_H
# include<config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#ifdef MPI
#include <mpi.h>
#endif
#ifdef OMP
# include <omp.h>
#endif
#include "global.h"
#include "git_hash.h"
#include "getopt.h"
#include "linalg_eo.h"
#include "geometry_eo.h"
#include "start.h"
/*#include "eigenvalues.h"*/
#include "measure_gauge_action.h"
#ifdef MPI
#include "xchange/xchange.h"
#endif
#include <io/utils.h>
#include "read_input.h"
#include "mpi_init.h"
#include "sighandler.h"
#include "boundary.h"
#include "solver/solver.h"
#include "init/init.h"
#include "smearing/stout.h"
#include "invert_eo.h"
#include "monomial/monomial.h"
#include "ranlxd.h"
#include "phmc.h"
#include "operator/D_psi.h"
#include "little_D.h"
#include "reweighting_factor.h"
#include "linalg/convert_eo_to_lexic.h"
#include "block.h"
#include "operator.h"
#include "sighandler.h"
#include "solver/dfl_projector.h"
#include "solver/generate_dfl_subspace.h"
#include "prepare_source.h"
#include <io/params.h>
#include <io/gauge.h>
#include <io/spinor.h>
#include <io/utils.h>
#include "solver/dirac_operator_eigenvectors.h"
#include "P_M_eta.h"
#include "operator/tm_operators.h"
#include "operator/Dov_psi.h"
#include "solver/spectral_proj.h"
#include "meas/measurements.h"
#include "source_generation.h"
#include "gettime.h"
#ifdef QUDA
#  include "quda_interface.h"
#endif

extern int nstore;
int check_geometry();

static void usage();
static void process_args(int argc, char *argv[], char ** input_filename, char ** filename);
static void set_default_filenames(char ** input_filename, char ** filename);

int main(int argc, char *argv[])
{
  FILE *parameterfile = NULL;
  int j, i, ix = 0, isample = 0, op_id = 0;
  char datafilename[206];
  char parameterfilename[206];
  char conf_filename[50];
  char basefilename[100];
  char plainbasefilename[100];
  char * input_filename = NULL;
  char * filename = NULL;
  double plaquette_energy;
  struct stout_parameters params_smear;
  spinor **s, *s_;
  spinor * psrc;
  spinor * pzn;
  spinor * bubbles[2];
  _Complex double * p_cplx_bbl;
  _Complex double * p_cplx_src;
  _Complex double * p_cplx_prp;

#ifdef _KOJAK_INST
#pragma pomp inst init
#pragma pomp inst begin(main)
#endif

#if (defined SSE || defined SSE2 || SSE3)
  signal(SIGILL, &catch_ill_inst);
#endif

  DUM_DERI = 8;
  DUM_MATRIX = DUM_DERI + 5;
#if ((defined BGL && defined XLC) || defined _USE_TSPLITPAR)
  NO_OF_SPINORFIELDS = DUM_MATRIX + 3;
#else
  NO_OF_SPINORFIELDS = DUM_MATRIX + 3;
#endif

  verbose = 0;
  g_use_clover_flag = 0;

#ifdef MPI

#  ifdef OMP
  int mpi_thread_provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &mpi_thread_provided);
#  else
  MPI_Init(&argc, &argv);
#  endif

  MPI_Comm_rank(MPI_COMM_WORLD, &g_proc_id);
#else
  g_proc_id = 0;
#endif

  double startTime = gettime();

  process_args(argc,argv,&input_filename,&filename);
  set_default_filenames(&input_filename, &filename);

  /* Read the input file */
  if( (j = read_input(input_filename)) != 0) {
    fprintf(stderr, "Could not find input file: %s\nAborting...\n", input_filename);
    exit(-1);
  }

#ifdef OMP
  init_openmp();
#endif

  if (!even_odd_flag) {
    even_odd_flag = 1;
    if (g_cart_id == 0) {
      printf("# Enforcing even_odd_flag = 1! \n");
      fflush(stdout);
    }
  }
  g_rgi_C1 = 0;
  if (Nsave == 0) {
    Nsave = 1;
  }

  if (g_running_phmc) {
    NO_OF_SPINORFIELDS = DUM_MATRIX + 8;
  }

  tmlqcd_mpi_init(argc, argv);

  g_dbw2rand = 0;

  /* starts the single and double precision random number */
  /* generator                                            */
  start_ranlux(rlxd_level, random_seed);

  /* we need to make sure that we don't have even_odd_flag = 1 */
  /* if any of the operators doesn't use it                    */
  /* in this way even/odd can still be used by other operators */
//  for(j = 0; j < no_operators; j++) if(!operator_list[j].even_odd_flag) even_odd_flag = 0;

#ifndef MPI
  g_dbw2rand = 0;
#endif

#ifdef _GAUGE_COPY
  j = init_gauge_field(VOLUMEPLUSRAND, 1);
#else
  j = init_gauge_field(VOLUMEPLUSRAND, 0);
#endif
  if (j != 0) {
    fprintf(stderr, "Not enough memory for gauge_fields! Aborting...\n");
    exit(-1);
  }
  j = init_geometry_indices(VOLUMEPLUSRAND);
  if (j != 0) {
    fprintf(stderr, "Not enough memory for geometry indices! Aborting...\n");
    exit(-1);
  }
  if (no_monomials > 0) {
    if (even_odd_flag) {
      j = init_monomials(VOLUMEPLUSRAND / 2, even_odd_flag);
    }
    else {
      j = init_monomials(VOLUMEPLUSRAND, even_odd_flag);
    }
    if (j != 0) {
      fprintf(stderr, "Not enough memory for monomial pseudo fermion fields! Aborting...\n");
      exit(-1);
    }
  }
  if (even_odd_flag) {
    j = init_spinor_field(VOLUMEPLUSRAND / 2, NO_OF_SPINORFIELDS+4);
  }
  else {
    j = init_spinor_field(VOLUMEPLUSRAND, NO_OF_SPINORFIELDS+2);
  }
  if (j != 0) {
    fprintf(stderr, "Not enough memory for spinor fields! Aborting...\n");
    exit(-1);
  }

  if (g_running_phmc) {
    j = init_chi_spinor_field(VOLUMEPLUSRAND / 2, 20);
    if (j != 0) {
      fprintf(stderr, "Not enough memory for PHMC Chi fields! Aborting...\n");
      exit(-1);
    }
  }

  if( (g_nproc_t*T)%dilutionblksz_t != 0) {
	  fprintf(stderr, "T needs to be a multiple of DilutionBlocksizeT...\n");
      exit(-1);
  }

  g_mu = g_mu1;

  if (g_cart_id == 0) {
    /*construct the filenames for the observables and the parameters*/
    strncpy(datafilename, filename, 200);
    strcat(datafilename, ".data");
    strncpy(parameterfilename, filename, 200);
    strcat(parameterfilename, ".para");

    parameterfile = fopen(parameterfilename, "w");
    write_first_messages(parameterfile, "invert", git_hash);
    fclose(parameterfile);
  }

  /* define the geometry */
  geometry();

  /* define the boundary conditions for the fermion fields */
  boundary(g_kappa);

  phmc_invmaxev = 1.;

  init_operators();

  /* list and initialize measurements*/
  if(g_proc_id == 0) {
    printf("\n");
    for(int j = 0; j < no_measurements; j++) {
      printf("# measurement id %d, type = %d\n", j, measurement_list[j].type);
    }
  }
  init_measurements();  

  /* this could be maybe moved to init_operators */
#ifdef _USE_HALFSPINOR
  j = init_dirac_halfspinor();
  if (j != 0) {
    fprintf(stderr, "Not enough memory for halffield! Aborting...\n");
    exit(-1);
  }
  if (g_sloppy_precision_flag == 1) {
    j = init_dirac_halfspinor32();
    if (j != 0)
    {
      fprintf(stderr, "Not enough memory for 32-bit halffield! Aborting...\n");
      exit(-1);
    }
  }
#  if (defined _PERSISTENT)
  if (even_odd_flag)
    init_xchange_halffield();
#  endif
#endif

  for (j = 0; j < Nmeas; j++) {

	if( strcmp(gauge_input_filename, "create_random_gaugefield") == 0 ) {
		random_gauge_field(reproduce_randomnumber_flag, g_gauge_field);
	}
	else {
		sprintf(conf_filename, "%s.%.4d", gauge_input_filename, nstore);
		if (g_cart_id == 0) {
		  printf("#\n# Trying to read gauge field from file %s in %s precision.\n",
				conf_filename, (gauge_precision_read_flag == 32 ? "single" : "double"));
		  fflush(stdout);
		}
		if( (i = read_gauge_field(conf_filename,g_gauge_field)) !=0) {
		  fprintf(stderr, "Error %d while reading gauge field from %s\n Aborting...\n", i, conf_filename);
		  exit(-2);
		}
		if (g_cart_id == 0) {
		  printf("# Finished reading gauge field.\n");
		  fflush(stdout);
		}
	}
#ifdef MPI
    xchange_gauge(g_gauge_field);
#endif

    /*compute the energy of the gauge field*/
    plaquette_energy = measure_plaquette( (const su3**) g_gauge_field);

    if (g_cart_id == 0) {
      printf("# The computed plaquette value is %e.\n", plaquette_energy / (6.*VOLUME*g_nproc));
      fflush(stdout);
    }

    if (use_stout_flag == 1){
      params_smear.rho = stout_rho;
      params_smear.iterations = stout_no_iter;
/*       if (stout_smear((su3_tuple*)(g_gauge_field[0]), &params_smear, (su3_tuple*)(g_gauge_field[0])) != 0) */
/*         exit(1) ; */
      g_update_gauge_copy = 1;
      plaquette_energy = measure_plaquette( (const su3**) g_gauge_field);

      if (g_cart_id == 0) {
        printf("# The plaquette value after stouting is %e\n", plaquette_energy / (6.*VOLUME*g_nproc));
        fflush(stdout);
      }
    }

    /* if any measurements are defined in the input file, do them here */
    measurement * meas;
    for(int imeas = 0; imeas < no_measurements; imeas++){
      meas = &measurement_list[imeas];
      if (g_proc_id == 0) {
        fprintf(stdout, "#\n# Beginning online measurement.\n");
      }
      meas->measurefunc(nstore, imeas, even_odd_flag);
    }

    if (reweighting_flag == 1) {
      reweighting_factor(reweighting_samples, nstore);
    }

    /* Compute minimal eigenvalues, if wanted */
    if (compute_evs != 0) {
      eigenvalues(&no_eigenvalues, 5000, eigenvalue_precision,
                  0, compute_evs, nstore, even_odd_flag);
    }
    if (phmc_compute_evs != 0) {
#ifdef MPI
      MPI_Finalize();
#endif
      return(0);
    }

    /* Compute the mode number or topological susceptibility using spectral projectors, if wanted*/

    if(compute_modenumber != 0 || compute_topsus !=0){
      
      s_ = calloc(no_sources_z2*VOLUMEPLUSRAND+1, sizeof(spinor));
      s  = calloc(no_sources_z2, sizeof(spinor*));
      if(s_ == NULL) { 
	printf("Not enough memory in %s: %d",__FILE__,__LINE__); exit(42);
      }
      if(s == NULL) { 
	printf("Not enough memory in %s: %d",__FILE__,__LINE__); exit(42);
      }
      
      
      for(i = 0; i < no_sources_z2; i++) {
#if (defined SSE3 || defined SSE2 || defined SSE)
        s[i] = (spinor*)(((unsigned long int)(s_)+ALIGN_BASE)&~ALIGN_BASE)+i*VOLUMEPLUSRAND;
#else
        s[i] = s_+i*VOLUMEPLUSRAND;
#endif

        random_spinor_field_lexic(s[i], reproduce_randomnumber_flag,RN_Z2);

/* 	what is this here needed for?? */
/*         spinor *aux_,*aux; */
/* #if ( defined SSE || defined SSE2 || defined SSE3 ) */
/*         aux_=calloc(VOLUMEPLUSRAND+1, sizeof(spinor)); */
/*         aux = (spinor *)(((unsigned long int)(aux_)+ALIGN_BASE)&~ALIGN_BASE); */
/* #else */
/*         aux_=calloc(VOLUMEPLUSRAND, sizeof(spinor)); */
/*         aux = aux_; */
/* #endif */

        if(g_proc_id == 0) {
          printf("source %d \n", i);
        }

        if(compute_modenumber != 0){
          mode_number(s[i], mstarsq);
        }

        if(compute_topsus !=0) {
          top_sus(s[i], mstarsq);
        }
      }
      free(s);
      free(s_);
    }


    /* move to operators as well */
    if (g_dflgcr_flag == 1) {
      /* set up deflation blocks */
      init_blocks(nblocks_t, nblocks_x, nblocks_y, nblocks_z);

      /* the can stay here for now, but later we probably need */
      /* something like init_dfl_solver called somewhere else  */
      /* create set of approximate lowest eigenvectors ("global deflation subspace") */

      /*       g_mu = 0.; */
      /*       boundary(0.125); */
      generate_dfl_subspace(g_N_s, VOLUME, reproduce_randomnumber_flag);
      /*       boundary(g_kappa); */
      /*       g_mu = g_mu1; */

      /* Compute little Dirac operators */
      /*       alt_block_compute_little_D(); */
      if (g_debug_level > 0) {
        check_projectors(reproduce_randomnumber_flag);
        check_local_D(reproduce_randomnumber_flag);
      }
      if (g_debug_level > 1) {
        check_little_D_inversion(reproduce_randomnumber_flag);
      }

    }
    if(SourceInfo.type == 1) {
      index_start = 0;
      index_end = 1;
    }

    g_precWS=NULL;
    if(use_preconditioning == 1){
      /* todo load fftw wisdom */
#if (defined HAVE_FFTW ) && !( defined MPI)
      loadFFTWWisdom(g_spinor_field[0],g_spinor_field[1],T,LX);
#else
      use_preconditioning=0;
#endif
    }

    if (g_cart_id == 0) {
      fprintf(stdout, "#\n"); /*Indicate starting of the operator part*/
    }
    for(op_id = 0; op_id < no_operators; op_id++) {
      boundary(operator_list[op_id].kappa);
      g_kappa = operator_list[op_id].kappa; 
      g_mu = 0.;

      if(use_preconditioning==1 && PRECWSOPERATORSELECT[operator_list[op_id].solver]!=PRECWS_NO ){
        printf("# Using preconditioning with treelevel preconditioning operator: %s \n",
              precWSOpToString(PRECWSOPERATORSELECT[operator_list[op_id].solver]));
        /* initial preconditioning workspace */
        operator_list[op_id].precWS=(spinorPrecWS*)malloc(sizeof(spinorPrecWS));
        spinorPrecWS_Init(operator_list[op_id].precWS,
                  operator_list[op_id].kappa,
                  operator_list[op_id].mu/2./operator_list[op_id].kappa,
                  -(0.5/operator_list[op_id].kappa-4.),
                  PRECWSOPERATORSELECT[operator_list[op_id].solver]);
        g_precWS = operator_list[op_id].precWS;

        if(PRECWSOPERATORSELECT[operator_list[op_id].solver] == PRECWS_D_DAGGER_D) {
          fitPrecParams(op_id);
        }
      }

      /* *************** START CALCULATING BUBBLES HERE *************** */

      no_samples = 1000;

      /* bubbles (g_spinor_field[6-7]) */
      bubbles[0] = g_spinor_field[6];
      bubbles[1] = g_spinor_field[7];

      /* need to modify the basename later */
      strcpy(plainbasefilename,SourceInfo.basename);

      int invcounter=1;

        for (int bs=0; bs<4; bs++)
          for (int bc=0; bc<3; bc++)
          {
        	/* initialize bubbles */
			zero_spinor_field(bubbles[0], VOLUME / 2);
			zero_spinor_field(bubbles[1], VOLUME / 2);

			/* we use g_spinor_field[0-7] for sources and props for the moment */
			/* 0-3 in case of 1 flavour  */
			/* 0-7 in case of 2 flavours */

			operator_list[op_id].sr0   = g_spinor_field[0];
			operator_list[op_id].sr1   = g_spinor_field[1];
			operator_list[op_id].prop0 = g_spinor_field[2];
			operator_list[op_id].prop1 = g_spinor_field[3];

			for(isample = 0; isample < no_samples; isample++) {

				/* get Z4 noise (store it in g_spinor_field[4-5]) */
				z4_volume_source(g_spinor_field[4], g_spinor_field[5], isample, nstore, (int)(100.0*operator_list[op_id].mu) );


            for (int bt=0; bt<T*g_nproc_t; bt+=dilutionblksz_t) {
				if (g_cart_id == 0) {
					fprintf(stdout, "#\n"); /*Indicate starting of new index*/
				}

				zero_spinor_field(operator_list[op_id].sr0, VOLUME / 2);
				zero_spinor_field(operator_list[op_id].sr1, VOLUME / 2);

				/* dilution */
				for(int t = 0; t < T; t++)
					for(int x = 0; x < LX; x++)
					  for(int y = 0; y < LY; y++)
						for(int z = 0; z < LZ; z++) {
						  i = g_lexic2eosub[ g_ipt[t][x][y][z] ];
						  if((t+x+y+z+g_proc_coords[3]*LZ+g_proc_coords[2]*LY
							+ g_proc_coords[0]*T+g_proc_coords[1]*LX)%2 == 0) {
							psrc = operator_list[op_id].sr0 + i;
							pzn = g_spinor_field[4] + i;
						  }
						  else {
							psrc = operator_list[op_id].sr1 + i;
							pzn = g_spinor_field[5] + i;
						  }
						  /* loop over dilution blocksize T */
						  for( int jt=bt; jt<bt+dilutionblksz_t; jt++ )
							  if( t+g_proc_coords[0]*T==jt ) {

								  if( bs==0 ) {
									  if( bc==0 )
										  psrc->s0.c0 = pzn->s0.c0;
									  else if( bc==1 )
										  psrc->s0.c1 = pzn->s0.c1;
									  else if( bc==2 )
										  psrc->s0.c2 = pzn->s0.c2;
								  }
								  else if( bs==1 ) {
									  if( bc==0 )
										  psrc->s1.c0 = pzn->s1.c0;
									  else if( bc==1 )
										  psrc->s1.c1 = pzn->s1.c1;
									  else if( bc==2 )
										  psrc->s1.c2 = pzn->s1.c2;
								  }
								  else if( bs==2 ) {
									  if( bc==0 )
										  psrc->s2.c0 = pzn->s2.c0;
									  else if( bc==1 )
										  psrc->s2.c1 = pzn->s2.c1;
									  else if( bc==2 )
										  psrc->s2.c2 = pzn->s2.c2;
								  }
								  else if( bs==3 ) {
									  if( bc==0 )
										  psrc->s3.c0 = pzn->s3.c0;
									  else if( bc==1 )
										  psrc->s3.c1 = pzn->s3.c1;
									  else if( bc==2 )
										  psrc->s3.c2 = pzn->s3.c2;
								  }
							  }
						} /* end dilution */
#ifdef MPI
						MPI_Barrier(MPI_COMM_WORLD);
#endif

#if 0
				// check sources here (for blocksz 2)
				printf("\n\n***************************\n");
				printf("*** bt=%d, bs=%d, bc=%d *** \n",bt,bs,bc);
				printf("(t,x,y,z) (is,ic)\n",bt,bs,bc);
				for(int t = 0; t < T; t++)
					for(int x = 0; x < LX; x++)
					  for(int y = 0; y < LY; y++)
						for(int z = 0; z < LZ; z++) {
						  i = g_lexic2eosub[ g_ipt[t][x][y][z] ];
						  if((t+x+y+z+g_proc_coords[3]*LZ+g_proc_coords[2]*LY
							+ g_proc_coords[0]*T+g_proc_coords[1]*LX)%2 == 0) {
							p_cplx_src[0] = (_Complex double*) (operator_list[op_id].sr0+i);
						  }
						  else {
							p_cplx_src[0] = (_Complex double*) (operator_list[op_id].sr1+i);
						  }
						  for( int is=0; is<4; is++ )
							for( int ic=0; ic<3; ic++ ) {
								if( cabs(p_cplx_src[0][is*3+ic]) > 0.0
										&& ( ((g_proc_coords[0]*T+t)!=bt && (g_proc_coords[0]*T+t)!=bt+1) || is!=bs || ic!=bc ) )
									printf("There is a problem! (%d,%d,%d,%d) (%d,%d): src = %f + I*%f\n", t,x,y,z,is,ic,creal(p_cplx_src[0][is*3+ic]),cimag(p_cplx_src[0][is*3+ic]));
								else if( cabs(p_cplx_src[0][is*3+ic]) == 0.0
										&& ( ((g_proc_coords[0]*T+t)==bt  || (g_proc_coords[0]*T+t)==bt+1) && is==bs && ic==bc ) )
									printf("There is a problem! (%d,%d,%d,%d) (%d,%d): src = %f + I*%f\n", t,x,y,z,is,ic,creal(p_cplx_src[0][is*3+ic]),cimag(p_cplx_src[0][is*3+ic]));
							}
						}
#endif
				//randomize initial guess for eigcg if needed-----experimental
				if( (operator_list[op_id].solver == INCREIGCG) && (operator_list[op_id].solver_params.eigcg_rand_guess_opt) ){ //randomize the initial guess
				  gaussian_volume_source( operator_list[op_id].prop0, operator_list[op_id].prop1,isample,ix,0); //need to check this
				}
				/* invert */
				if(g_proc_id == 0)
					printf("\n# Starting inversion %d/%d\n", invcounter++, 12*no_samples*((g_nproc_t*T)/dilutionblksz_t));
				operator_list[op_id].inverter(op_id, index_start, 0);

				/* gather bubbles */
				for(int t = 0; t < T; t++)
					for(int x = 0; x < LX; x++)
					  for(int y = 0; y < LY; y++)
						for(int z = 0; z < LZ; z++) {
						  i = g_lexic2eosub[ g_ipt[t][x][y][z] ];
						  if((t+x+y+z+g_proc_coords[3]*LZ+g_proc_coords[2]*LY
							+ g_proc_coords[0]*T+g_proc_coords[1]*LX)%2 == 0)
						  {
							  p_cplx_bbl = (_Complex double*) (bubbles[0]+i);
							  p_cplx_src = (_Complex double*) (operator_list[op_id].sr0);
							  p_cplx_prp = (_Complex double*) (operator_list[op_id].prop0+i);
						  }
						  else {
							  p_cplx_bbl = (_Complex double*) (bubbles[1]+i);
							  p_cplx_src = (_Complex double*) (operator_list[op_id].sr0);
							  p_cplx_prp = (_Complex double*) (operator_list[op_id].prop1+i);
						  }

					for( int is=0; is<4; is++ )
						for( int ic=0; ic<3; ic++ ) {
							p_cplx_bbl[is*3+ic] += p_cplx_prp[is*3+ic] * conj(p_cplx_src[bs*3+bc]) / (double)no_samples;
						}
				}
			  } /* end bt */
            } /* end isample */

              /* write bubbles for current bs,bc */
              operator_list[op_id].prop0 = bubbles[0];
              operator_list[op_id].prop1 = bubbles[1];

              sprintf(basefilename, "%s.s%dc%d", plainbasefilename, bs, bc);
              SourceInfo.basename = basefilename;
              SourceInfo.nstore = nstore;
              operator_list[op_id].write_prop( op_id, 0, 0 );
          } /* end bs,bc */

      /* ***************  END CALCULATING BUBBLES HERE  *************** */


      if(use_preconditioning==1 && operator_list[op_id].precWS!=NULL ){
        /* free preconditioning workspace */
        spinorPrecWS_Free(operator_list[op_id].precWS);
        free(operator_list[op_id].precWS);
      }

      if(operator_list[op_id].type == OVERLAP){
        free_Dov_WS();
      }

    }
    nstore += Nsave;
  }


#ifdef OMP
  free_omp_accumulators();
#endif
  free_blocks();
  free_dfl_subspace();
  free_gauge_field();
  free_geometry_indices();
  free_spinor_field();
  free_moment_field();
  free_chi_spinor_field();
  free(filename);
  free(input_filename);
#ifdef QUDA
  _endQuda();
#endif
  double endTime = gettime();
  double diffTime = endTime - startTime;
  if(g_proc_id == 0)
    printf("# Total time elapsed: %f secs\n", diffTime);
#ifdef MPI
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
#endif
  return(0);
#ifdef _KOJAK_INST
#pragma pomp inst end(main)
#endif
}

static void usage()
{
  fprintf(stdout, "Inversion for EO preconditioned Wilson twisted mass QCD\n");
  fprintf(stdout, "Version %s \n\n", PACKAGE_VERSION);
  fprintf(stdout, "Please send bug reports to %s\n", PACKAGE_BUGREPORT);
  fprintf(stdout, "Usage:   invert [options]\n");
  fprintf(stdout, "Options: [-f input-filename]\n");
  fprintf(stdout, "         [-o output-filename]\n");
  fprintf(stdout, "         [-v] more verbosity\n");
  fprintf(stdout, "         [-h|-? this help]\n");
  fprintf(stdout, "         [-V] print version information and exit\n");
  exit(0);
}

static void process_args(int argc, char *argv[], char ** input_filename, char ** filename) {
  int c;
  while ((c = getopt(argc, argv, "h?vVf:o:")) != -1) {
    switch (c) {
      case 'f':
        *input_filename = calloc(200, sizeof(char));
        strncpy(*input_filename, optarg, 200);
        break;
      case 'o':
        *filename = calloc(200, sizeof(char));
        strncpy(*filename, optarg, 200);
        break;
      case 'v':
        verbose = 1;
        break;
      case 'V':
        if(g_proc_id == 0) {
          fprintf(stdout,"%s %s\n",PACKAGE_STRING,git_hash);
        }
        exit(0);
        break;
      case 'h':
      case '?':
      default:
        if( g_proc_id == 0 ) {
          usage();
        }
        break;
    }
  }
}

static void set_default_filenames(char ** input_filename, char ** filename) {
  if( *input_filename == NULL ) {
    *input_filename = calloc(13, sizeof(char));
    strcpy(*input_filename,"invert.input");
  }
  
  if( *filename == NULL ) {
    *filename = calloc(7, sizeof(char));
    strcpy(*filename,"output");
  } 
}

