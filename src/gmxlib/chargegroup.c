/* -*- mode: c; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; c-file-style: "stroustrup"; -*-
 *
 * 
 *                This source code is part of
 * 
 *                 G   R   O   M   A   C   S
 * 
 *          GROningen MAchine for Chemical Simulations
 * 
 *                        VERSION 3.2.0
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2004, The GROMACS development team,
 * check out http://www.gromacs.org for more information.

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 * 
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 * 
 * For more info, check our website at http://www.gromacs.org
 * 
 * And Hey:
 * GROningen Mixture of Alchemy and Childrens' Stories
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include "sysstuff.h"
#include "typedefs.h"
#include "vec.h"
#include "pbc.h"
#include "smalloc.h"
#include "gmx_fatal.h"
#include "chargegroup.h"


void calc_chargegroup_radii(const gmx_mtop_t *mtop,rvec *x,
                            real *rvdw1,real *rvdw2,
                            real *rcoul1,real *rcoul2)
{
    real r2v1,r2v2,r2c1,r2c2,r2;
    int  mb,m,cg,a_mol,a0,a1,a;
    t_iparams *ip;
    gmx_molblock_t *molb;
    gmx_moltype_t *molt;
    t_block *cgs;
    t_atom *atom;
    rvec cen;

    r2v1 = 0;
    r2v2 = 0;
    r2c1 = 0;
    r2c2 = 0;

    ip = mtop->ffparams.iparams;

    a_mol = 0;
    for(mb=0; mb<mtop->nmolblock; mb++)
    {
        molb = &mtop->molblock[mb];
        molt = &mtop->moltype[molb->type];
        cgs  = &molt->cgs;
        atom = molt->atoms.atom;
        for(m=0; m<molb->nmol; m++)
        {
            for(cg=0; cg<cgs->nr; cg++)
            {
                a0 = cgs->index[cg];
                a1 = cgs->index[cg+1];
                if (a1 - a0 > 1)
                {
                    clear_rvec(cen);
                    for(a=a0; a<a1; a++)
                    {
                        rvec_inc(cen,x[a_mol+a]);
                    }
                    svmul(1.0/(a1-a0),cen,cen);
                    for(a=a0; a<a1; a++)
                    {
                        r2 = distance2(cen,x[a_mol+a]);
                        if (r2 > r2v2 &&
                            (ip[atom[a].type ].lj.c6  != 0 ||
                             ip[atom[a].type ].lj.c12 != 0 ||
                             ip[atom[a].typeB].lj.c6  != 0 ||
                             ip[atom[a].typeB].lj.c12 != 0))
                        {
                            if (r2 > r2v1)
                            {
                                r2v2 = r2v1;
                                r2v1 = r2;
                            }
                            else
                            {
                                r2v2 = r2;
                            }
                        }
                        if (r2 > r2c2 &&
                            (atom[a].q != 0 || atom[a].qB != 0))
                        {
                            if (r2 > r2c1)
                            {
                                r2c2 = r2c1;
                                r2c1 = r2;
                            }
                            else
                            {
                                r2c2 = r2;
                            }
                        }
                    }
                }
            }
            a_mol += molb->natoms_mol;
        }
    }

    *rvdw1  = sqrt(r2v1);
    *rvdw2  = sqrt(r2v2);
    *rcoul1 = sqrt(r2c1);
    *rcoul2 = sqrt(r2c2);
}

void calc_cgcm(FILE *fplog,int cg0,int cg1,t_block *cgs,
               rvec pos[],rvec cg_cm[])
{
    int  icg,k,k0,k1,d;
    rvec cg;
    real nrcg,inv_ncg;
    atom_id *cgindex;

#ifdef DEBUG
    fprintf(fplog,"Calculating centre of geometry for charge groups %d to %d\n",
            cg0,cg1);
#endif
    cgindex = cgs->index;

    /* Compute the center of geometry for all charge groups */
    for(icg=cg0; (icg<cg1); icg++) {
        k0      = cgindex[icg];
        k1      = cgindex[icg+1];
        nrcg    = k1-k0;
        if (nrcg == 1) {
            copy_rvec(pos[k0],cg_cm[icg]);
        }
        else {
            inv_ncg = 1.0/nrcg;

            clear_rvec(cg);
            for(k=k0; (k<k1); k++)  {
                for(d=0; (d<DIM); d++)
                    cg[d] += pos[k][d];
            }
            for(d=0; (d<DIM); d++)
                cg_cm[icg][d] = inv_ncg*cg[d];
        }
    }
}

void put_charge_groups_in_box(FILE *fplog,int cg0,int cg1,
                              int ePBC,matrix box,t_block *cgs,
                              rvec pos[],rvec cg_cm[])

{ 
    int  npbcdim,icg,k,k0,k1,d,e;
    rvec cg;
    real nrcg,inv_ncg;
    atom_id *cgindex;
    gmx_bool bTric;

    if (ePBC == epbcNONE) 
        gmx_incons("Calling put_charge_groups_in_box for a system without PBC");

#ifdef DEBUG
    fprintf(fplog,"Putting cgs %d to %d in box\n",cg0,cg1);
#endif
    cgindex = cgs->index;

    if (ePBC == epbcXY)
        npbcdim = 2;
    else
        npbcdim = 3;

    bTric = TRICLINIC(box);

    for(icg=cg0; (icg<cg1); icg++) {
        /* First compute the center of geometry for this charge group */
        k0      = cgindex[icg];
        k1      = cgindex[icg+1];
        nrcg    = k1-k0;

        if (nrcg == 1) {
            copy_rvec(pos[k0],cg_cm[icg]);
        } else {
            inv_ncg = 1.0/nrcg;

            clear_rvec(cg);
            for(k=k0; (k<k1); k++)  {
                for(d=0; d<DIM; d++)
                    cg[d] += pos[k][d];
            }
            for(d=0; d<DIM; d++)
                cg_cm[icg][d] = inv_ncg*cg[d];
        }
        /* Now check pbc for this cg */
        if (bTric) {
            for(d=npbcdim-1; d>=0; d--) {
                while(cg_cm[icg][d] < 0) {
                    for(e=d; e>=0; e--) {
                        cg_cm[icg][e] += box[d][e];
                        for(k=k0; (k<k1); k++) 
                            pos[k][e] += box[d][e];
                    }
                }
                while(cg_cm[icg][d] >= box[d][d]) {
                    for(e=d; e>=0; e--) {
                        cg_cm[icg][e] -= box[d][e];
                        for(k=k0; (k<k1); k++) 
                            pos[k][e] -= box[d][e];
                    }
                }
            }
        } else {
            for(d=0; d<npbcdim; d++) {
                while(cg_cm[icg][d] < 0) {
                    cg_cm[icg][d] += box[d][d];
                    for(k=k0; (k<k1); k++) 
                        pos[k][d] += box[d][d];
                }
                while(cg_cm[icg][d] >= box[d][d]) {
                    cg_cm[icg][d] -= box[d][d];
                    for(k=k0; (k<k1); k++) 
                        pos[k][d] -= box[d][d];
                }
            }
        }
#ifdef DEBUG_PBC
        for(d=0; (d<npbcdim); d++) {
            if ((cg_cm[icg][d] < 0) || (cg_cm[icg][d] >= box[d][d]))
                gmx_fatal(FARGS,"cg_cm[%d] = %15f  %15f  %15f\n"
                          "box = %15f  %15f  %15f\n",
                          icg,cg_cm[icg][XX],cg_cm[icg][YY],cg_cm[icg][ZZ],
                          box[XX][XX],box[YY][YY],box[ZZ][ZZ]);
        }
#endif
    }
}
