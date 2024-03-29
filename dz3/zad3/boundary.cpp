#include "boundary.h"
#include <stdio.h>

//grid is parallelised in the x direction

void boundarypsi(double* psi, int m, int n, int b, int h, int w)
{

	int i,j;

	//BCs on bottom edge

	for (i=b+1;i<=b+w-1;i++)
	{
		psi[i*(m+2)+0] = (double)(i-b);
	}

	for (i=b+w;i<=m;i++)
	{
		psi[i*(m+2)+0] = (double)(w);
	}

	//BCS on RHS

	for (j=1; j <= h; j++)
	{
		psi[(m+1)*(m+2)+j] = (double) w;
	}

	for (j=h+1;j<=h+w-1; j++)
	{
		psi[(m+1)*(m+2)+j]=(double)(w-j+h);
	}
}
