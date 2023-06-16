#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <stdexcept>

#include "arraymalloc.h"
#include "boundary.h"
#include "jacobi_opencl.hh"
#include "cfdio.h"


int main(int argc, char **argv)
{
	int printfreq=1000; //output frequency
	double error, bnorm;

	//command line arguments
	int scalefactor, numiter;

	//simulation sizes
	int bbase=10;
	int hbase=15;
	int wbase=5;
	int mbase=32;
	int nbase=32;

	int irrotational = 1;

	int m,n,b,h,w;
	int iter;
	int i,j;

	double tstart, tstop, ttot, titer;

	//check command line parameters and parse them

	if (argc < 3) {
		printf("Usage: cfd <scale> <numiter>\n");
		return -1;
	}

	scalefactor=atoi(argv[1]);
	numiter=atoi(argv[2]);

	int batch_size{1};
	if (argc > 3) {
		batch_size = atoi(argv[3]);
	}

	printf("Scale Factor = %i, iterations = %i\n",scalefactor, numiter);

	printf("Irrotational flow\n");

	//Calculate b, h & w and m & n
	b = bbase*scalefactor;
	h = hbase*scalefactor;
	w = wbase*scalefactor;
	m = mbase*scalefactor;
	n = nbase*scalefactor;

	printf("Running CFD on %d x %d grid in serial\n",m,n);

	//allocate arrays
	std::vector<double> psi((m + 2) * (n + 2), 0.0);
	std::vector<double> psitmp((m + 2) * (n + 2), 0.0);
	
	//set the psi boundary conditions
	boundarypsi(psi.data(),m,n,b,h,w);

	//compute normalisation factor for error
	bnorm=0.0;

	for (i=0;i<m+2;i++) {
		for (j=0;j<n+2;j++) {
			bnorm += psi[i*(m+2)+j]*psi[i*(m+2)+j];
		}
	}
	bnorm=sqrt(bnorm);

	Jacobi jacobi;
	try {
		jacobi = Jacobi(m, n, psitmp.data(), psi.data());
	} catch (std::exception e) {
		std::cerr << "Error constructing the Jacobi object: " << e.what() << std::endl;
		return -1;
	}

	//begin iterative Jacobi loop
	printf("\nStarting main loop...\n\n");
	tstart=gettime();

	for (iter = 1; iter <= numiter; iter++) {
		//copy back
		for(i=1;i<=m;i++) {
			for(j=1;j<=n;j++) {
				psi[i*(m+2)+j]=psitmp[i*(m+2)+j];
			}
		}

		//calculate psi for next iteration
		jacobi.step();
	
		//print loop information
		if(iter%printfreq == 0) {
			printf("Completed iteration %d\n",iter);
		}
	}

	//calculate current error
	error = sqrt(deltasq(psitmp,psi,m,n)) / bnorm;

	if (iter > numiter) iter=numiter;

	tstop=gettime();

	ttot=tstop-tstart;
	titer=ttot/(double)iter;

	//print out some stats
	printf("\n... finished\n");
	printf("After %d iterations, the error is %g\n",iter,error);
	printf("Time for %d iterations was %g seconds\n",iter,ttot);
	printf("Each iteration took %g seconds\n",titer);

	printf("... finished\n");

	return 0;
}
