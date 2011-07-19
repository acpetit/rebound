#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "main.h"
#include "particle.h"
#include "boundaries.h"
#include "output.h"


double total_energy();
double energy_initial;

#ifdef GRAVITY_TREE
extern double opening_angle2;
#else // GRAVITY_TREE
double opening_angle2;
#endif // GRAVITY_TREE

void problem_init(int argc, char* argv[]){
	// Setup particle structures
	if (argc>1){
		opening_angle2 = atof(argv[1]);
		opening_angle2 *= opening_angle2;
	}
	boxsize = 2;
	softening =boxsize/100.;
	tmax = 1.;
	init_particles(200);
	dt = 1e-4;
	// Initial conditions
	for (int i =0;i<N;i++){
		double r;
		double rmax = 0.1*boxsize;
		do{
			particles[i].x = ((double)rand()/(double)RAND_MAX-0.5)*boxsize_x;
			particles[i].y = ((double)rand()/(double)RAND_MAX-0.5)*boxsize_y;
			particles[i].z = ((double)rand()/(double)RAND_MAX-0.5)*boxsize_z;
			r = sqrt(particles[i].x*particles[i].x+particles[i].y*particles[i].y+particles[i].z*particles[i].z);
		}while(r>rmax);
		double M = pow(r/rmax,3.);
		double vkep = sqrt(G*M/r);
		double phi = 2.*M_PI*((double)rand()/(double)RAND_MAX-0.5);
		double psi = 2.*M_PI*((double)rand()/(double)RAND_MAX-0.5);
		particles[i].vx = vkep*sin(phi)*sin(psi)   +vkep*0.1*((double)rand()/(double)RAND_MAX-0.5);
		particles[i].vy = vkep*cos(phi)*sin(psi)   +vkep*0.1*((double)rand()/(double)RAND_MAX-0.5);
		particles[i].vz = vkep*cos(psi)            +vkep*0.1*((double)rand()/(double)RAND_MAX-0.5);
		particles[i].ax = 0;
		particles[i].ay = 0;
		particles[i].az = 0;
		particles[i].m = 1./(double)N;
	}
	energy_initial = total_energy();
	// No ghost boxes 
	nghostx = 0;
	nghosty = 0;
	nghostz = 0;
}

double total_energy(){
	double energy = 0;
	for (int i=0;i<N;i++){
		struct particle p1 = particles[i];
		energy += 0.5*p1.m*(p1.vx*p1.vx + p1.vy*p1.vy + p1.vz*p1.vz);
		for (int j=i+1;j<N;j++){
			struct particle p2 = particles[j];
			double dx = p2.x - p1.x;
			double dy = p2.y - p1.y;
			double dz = p2.z - p1.z;
			energy -= G*p1.m*p2.m/sqrt(dx*dx + dy*dy + dz*dz + softening*softening);
		}
	}
	return energy;
}

void problem_inloop(){
}

void problem_output(){
}

void problem_finish(){
	FILE* of = fopen("error.txt","a+"); 
	double error= fabs((energy_initial-total_energy())/energy_initial);
	struct timeval tim;
	gettimeofday(&tim, NULL);
	double timing_final = tim.tv_sec+(tim.tv_usec/1000000.0);
	double error_limit = 1e-4;
	int pass = (error>error_limit?0:1);
	fprintf(of,"%d\t%e\t%e\t%e\t%d\t%e\t",N,dt,error,error_limit,pass,timing_final-timing_initial);
	fprintf(of,"N = %d, opening_angle = %f",N,sqrt(opening_angle2));
	fprintf(of,"\n");
	fclose(of);
}
