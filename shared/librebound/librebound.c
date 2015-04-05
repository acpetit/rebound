/**
 * @file 	librebound.c
 * @brief 	Declarations and functions for shared library access to the IAS15 and MIKKOLA integrators.
 * @author 	Hanno Rein <hanno@hanno-rein.de>
 * 		Dave Spiegel <dave@ias.edu>
 * 
 * @section 	LICENSE
 * Copyright (c) 2011 Hanno Rein, Dave Spiegel
 *
 * This file is part of rebound.
 *
 * rebound is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rebound is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rebound.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "particle.h"
#include "gravity.h"
#include "particle.h"
#include "main.h"
#include "boundaries.h"
#include "integrator.h"

// Default values of parameters and constants
double dt 	= 0.01;	
double t 	= 0;
double tmax	= 0;
double G 	= 1;
double softening = 0;
extern int Nmax;	
extern int iter;  // TODO DEBUG

// Chooses which integrator to use.
// 0: IAS15 (default)
// 1: MIKKOLA
// 2: WH
// 3: Leap-frog
int selected_integrator = 0; 

extern void integrator_ias15_part1();
extern void integrator_ias15_part2();
extern void integrator_ias15_reset();
extern void integrator_mikkola_part1();
extern void integrator_mikkola_part2();
extern void integrator_mikkola_reset();
extern void integrator_wh_part1();
extern void integrator_wh_part2();
extern void integrator_wh_reset();
extern void integrator_leapfrog_part1();
extern void integrator_leapfrog_part2();
extern void integrator_leapfrog_reset();

// Function pointer to additional forces
void (*problem_additional_forces) () = NULL;

// Particle structure
struct particle* particles = NULL;

// Particle getter/setter methods.
void setp(struct particle* _p){
	free(particles);
	particles = malloc(sizeof(struct particle)*N);
	memcpy(particles,_p,sizeof(struct particle)*N);
}
struct particle particle_get(int i){
	return particles[i];
}
struct particle* particles_get(){
	return particles;
}
void set_additional_forces(void (* _cb)()){
	problem_additional_forces = _cb;
}
double timing;
// Integrate for 1 step

void integrator_part1(){
	switch(selected_integrator){
		case 1:
			integrator_mikkola_part1();
			break;
		case 2:
			integrator_wh_part1();
			break;
		case 3:
			integrator_leapfrog_part1();
			break;
		default:
			integrator_ias15_part1();
			break;
	}
}
void integrator_part2(){
	switch(selected_integrator){
		case 1:
			integrator_mikkola_part2();
			break;
		case 2:
			integrator_wh_part2();
			break;
		case 3:
			integrator_leapfrog_part2();
			break;
		default:
			integrator_ias15_part2();
			break;
	}
}

void rebound_step(){ 
	struct timeval tim;
	gettimeofday(&tim, NULL);
	double timing_initial = tim.tv_sec+(tim.tv_usec/1000000.0);
	if (N<=0){
		fprintf(stderr,"\n\033[1mError!\033[0m No particles found. Exiting.\n");
		return;
	}
	integrator_part1();
	gravity_calculate_acceleration();
	if (N_megno){
		gravity_calculate_variational_acceleration();
	}
	if (problem_additional_forces) problem_additional_forces();
	integrator_part2();
	gettimeofday(&tim, NULL);
	double timing_final = tim.tv_sec+(tim.tv_usec/1000000.0);
	timing = timing_final-timing_initial;
}

// Integrate for 1 step
void reset(){ 
	dt 		= 0.01;
	t 		= 0;
	tmax		= 0;
	G 		= 1;
	softening 	= 0;
	N 		= 0;
	Nmax 		= 0;
	N_active 	= -1;
	N_megno 	= 0;
	iter		= 0;
	free(particles);
	particles 	= NULL;
	integrator_ias15_reset();
	integrator_mikkola_reset();
	integrator_wh_reset();
	integrator_leapfrog_reset();
	struct timeval tim;
	gettimeofday(&tim, NULL);
	srand ( tim.tv_usec + getpid());
}

int check_eject(){
	// TODO: Remove this.
	double x = particles[1].x;
	double y = particles[1].y;
	double z = particles[1].z;
	if (sqrt(x*x + y*y + z*z)>100.){
		return 1;
	}
	return 0;
}
// Integrate until t=_tmax
void integrate(double _tmax, int exactFinishTime){
	struct timeval tim;
	gettimeofday(&tim, NULL);
	double timing_initial = tim.tv_sec+(tim.tv_usec/1000000.0);
	tmax = _tmax;
	double dt_last_done = dt;
	int last_step = 0;
	while(t<tmax && last_step<2){
		if (N<=0){
			fprintf(stderr,"\n\033[1mError!\033[0m No particles found. Exiting.\n");
			return;
		}
		double t_beginning = t;	
		integrator_part1();
		if (t_beginning+dt>=tmax || last_step){
			integrator_synchronized = 1;
		}else{
			integrator_synchronized = 0;
		}
		gravity_calculate_acceleration();
		if (N_megno){
			gravity_calculate_variational_acceleration();
		}
		if (problem_additional_forces) problem_additional_forces();
		integrator_part2();
		
		if (check_eject()){
			return;
		}
		if (t+dt>=tmax && exactFinishTime==1){
			dt = tmax-t;
			last_step++;
		}else{
			dt_last_done = dt;
		}
	}
	dt = dt_last_done;
	gettimeofday(&tim, NULL);
	double timing_final = tim.tv_sec+(tim.tv_usec/1000000.0);
	timing = timing_final-timing_initial;
}
	 
