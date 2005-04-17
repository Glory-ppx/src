/* Convert RMS to interval velocity using LS and shaping regularization.

Takes: rect1= rect2= ...

rectN defines the size of the smoothing stencil in N-th dimension.
*/
/*
  Copyright (C) 2004 University of Texas at Austin
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <math.h>

#include <rsf.h>

#include "smoothder.h"
#include "repeat.h"

int main(int argc, char* argv[])
{
    int i, j, niter, nd, dim, n1, n2, i1, i2;
    int n[SF_MAX_DIM], box[SF_MAX_DIM], **rct;
    float **vr, **vi, **wt, **v0, wti;
    char key[6];
    bool nonstat;
    sf_file vrms, vint, weight, vout, rect[SF_MAX_DIM];

    sf_init(argc,argv);
    vrms = sf_input("in");
    vint = sf_output("out");
    weight = sf_input("weight");

    dim = sf_filedims (vrms,n);

    nd = 1;
    for (i=0; i < dim; i++) {
	nd *= n[i];
    }
    n1 = n[0];
    n2 = nd/n1;
    
    if (!sf_getbool("nonstat",&nonstat)) nonstat=false;
    /* if y, use nonstationary smoothing */

    rct = nonstat? sf_intalloc2(nd,dim): NULL;

    for (i=0; i < dim; i++) {
	snprintf(key,6,"rect%d",i+1);
	if (nonstat) {
	    box[i]=1;
	    if (NULL != sf_getstring(key)) {
		rect[i] = sf_input(key);
		if (SF_INT != sf_gettype(rect[i])) 
		    sf_error("Need int %s",key);
		sf_intread(rct[i],nd,rect[i]);
		sf_fileclose(rect[i]);
		for (j=0; j < nd; j++) {
		    if (rct[i][j] > box[i]) box[i] = rct[i][j];
		}
	    } else {
		for (j=0; j < nd; j++) {
		    rct[i][j]=1;
		}
	    }
	} else {		
	    if (!sf_getint(key,box+i)) box[i]=1;
	}
    }

    smoothder_init(nd, dim, box, n, rct);

    vr = sf_floatalloc2(n1,n2);
    vi = sf_floatalloc2(n1,n2);
    wt = sf_floatalloc2(n1,n2);
    v0 = sf_floatalloc2(n1,n2);

    sf_floatread(vr[0],nd,vrms);
    sf_floatread(wt[0],nd,weight);

    if (!sf_getint("niter",&niter)) niter=100;
    /* maximum number of iterations */

    wti = 0.;
    for (i2=0; i2 < n2; i2++) {
	for (i1=0; i1 < n1; i1++) {
	    wti += wt[i2][i1]*wt[i2][i1];
	}
    }
    if (wti > 0.) wti = sqrtf(n1*n2/wti);

    for (i2=0; i2 < n2; i2++) {
	for (i1=0; i1 < n1; i1++) {
	    vr[i2][i1] *= vr[i2][i1]*(i1+1.); /* vrms^2*t - data */
	    wt[i2][i1] *= wti/(i1+1.); /* decrease weight with time */	 
	    v0[i2][i1] = -vr[i2][0];
	}
    }
    
    repeat_lop(false,true,nd,nd,v0[0],vr[0]);
    smoothder(niter, wt[0], vr[0], vi[0]);
 
    for (i2=0; i2 < n2; i2++) {
	for (i1=0; i1 < n1; i1++) {
	    vi[i2][i1] -= v0[i2][i1];
	}
    }

    repeat_lop(false,false,nd,nd,vi[0],vr[0]);

    for (i2=0; i2 < n2; i2++) {
	for (i1=0; i1 < n1; i1++) {
	    vr[i2][i1] = sqrtf(fabsf(vr[i2][i1]/(i1+1.)));
	    vi[i2][i1] = sqrtf(fabsf(vi[i2][i1]));
	}
    }

    sf_floatwrite(vi[0],nd,vint);

    if (NULL != sf_getstring("vrmsout")) {
	/* optionally, output predicted vrms */
	vout = sf_output("vrmsout");

	sf_floatwrite(vr[0],nd,vout);
    }

    exit(0);
}

/* 	$Id$	 */
