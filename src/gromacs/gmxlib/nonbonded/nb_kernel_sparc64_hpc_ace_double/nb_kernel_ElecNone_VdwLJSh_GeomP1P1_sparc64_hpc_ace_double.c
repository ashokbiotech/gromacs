/*
 * This file is part of the GROMACS molecular simulation package.
 *
 * Copyright (c) 2012,2013,2014, by the GROMACS development team, led by
 * Mark Abraham, David van der Spoel, Berk Hess, and Erik Lindahl,
 * and including many others, as listed in the AUTHORS file in the
 * top-level source directory and at http://www.gromacs.org.
 *
 * GROMACS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * GROMACS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GROMACS; if not, see
 * http://www.gnu.org/licenses, or write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *
 * If you want to redistribute modifications to GROMACS, please
 * consider that scientific software is very special. Version
 * control is crucial - bugs must be traceable. We will be happy to
 * consider code for inclusion in the official distribution, but
 * derived work must not be called official GROMACS. Details are found
 * in the README & COPYING files - if they are missing, get the
 * official version at http://www.gromacs.org.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the research papers on the package. Check out http://www.gromacs.org.
 */
/*
 * Note: this file was generated by the GROMACS sparc64_hpc_ace_double kernel generator.
 */
#include "config.h"

#include <math.h>

#include "../nb_kernel.h"
#include "gromacs/legacyheaders/types/simple.h"
#include "gromacs/math/vec.h"
#include "gromacs/legacyheaders/nrnb.h"

#include "kernelutil_sparc64_hpc_ace_double.h"

/*
 * Gromacs nonbonded kernel:   nb_kernel_ElecNone_VdwLJSh_GeomP1P1_VF_sparc64_hpc_ace_double
 * Electrostatics interaction: None
 * VdW interaction:            LennardJones
 * Geometry:                   Particle-Particle
 * Calculate force/pot:        PotentialAndForce
 */
void
nb_kernel_ElecNone_VdwLJSh_GeomP1P1_VF_sparc64_hpc_ace_double
                    (t_nblist                    * gmx_restrict       nlist,
                     rvec                        * gmx_restrict          xx,
                     rvec                        * gmx_restrict          ff,
                     t_forcerec                  * gmx_restrict          fr,
                     t_mdatoms                   * gmx_restrict     mdatoms,
                     nb_kernel_data_t gmx_unused * gmx_restrict kernel_data,
                     t_nrnb                      * gmx_restrict        nrnb)
{
    /* Suffixes 0,1,2,3 refer to particle indices for waters in the inner or outer loop, or
     * just 0 for non-waters.
     * Suffixes A,B refer to j loop unrolling done with double precision SIMD, e.g. for the two different
     * jnr indices corresponding to data put in the four positions in the SIMD register.
     */
    int              i_shift_offset,i_coord_offset,outeriter,inneriter;
    int              j_index_start,j_index_end,jidx,nri,inr,ggid,iidx;
    int              jnrA,jnrB;
    int              j_coord_offsetA,j_coord_offsetB;
    int              *iinr,*jindex,*jjnr,*shiftidx,*gid;
    real             rcutoff_scalar;
    real             *shiftvec,*fshift,*x,*f;
    _fjsp_v2r8       tx,ty,tz,fscal,rcutoff,rcutoff2,jidxall;
    int              vdwioffset0;
    _fjsp_v2r8       ix0,iy0,iz0,fix0,fiy0,fiz0,iq0,isai0;
    int              vdwjidx0A,vdwjidx0B;
    _fjsp_v2r8       jx0,jy0,jz0,fjx0,fjy0,fjz0,jq0,isaj0;
    _fjsp_v2r8       dx00,dy00,dz00,rsq00,rinv00,rinvsq00,r00,qq00,c6_00,c12_00;
    int              nvdwtype;
    _fjsp_v2r8       rinvsix,rvdw,vvdw,vvdw6,vvdw12,fvdw,fvdw6,fvdw12,vvdwsum,sh_vdw_invrcut6;
    int              *vdwtype;
    real             *vdwparam;
    _fjsp_v2r8       one_sixth   = gmx_fjsp_set1_v2r8(1.0/6.0);
    _fjsp_v2r8       one_twelfth = gmx_fjsp_set1_v2r8(1.0/12.0);
    _fjsp_v2r8       itab_tmp;
    _fjsp_v2r8       dummy_mask,cutoff_mask;
    _fjsp_v2r8       one     = gmx_fjsp_set1_v2r8(1.0);
    _fjsp_v2r8       two     = gmx_fjsp_set1_v2r8(2.0);
    union { _fjsp_v2r8 simd; long long int i[2]; } vfconv,gbconv,ewconv;

    x                = xx[0];
    f                = ff[0];

    nri              = nlist->nri;
    iinr             = nlist->iinr;
    jindex           = nlist->jindex;
    jjnr             = nlist->jjnr;
    shiftidx         = nlist->shift;
    gid              = nlist->gid;
    shiftvec         = fr->shift_vec[0];
    fshift           = fr->fshift[0];
    nvdwtype         = fr->ntype;
    vdwparam         = fr->nbfp;
    vdwtype          = mdatoms->typeA;

    rcutoff_scalar   = fr->rvdw;
    rcutoff          = gmx_fjsp_set1_v2r8(rcutoff_scalar);
    rcutoff2         = _fjsp_mul_v2r8(rcutoff,rcutoff);

    sh_vdw_invrcut6  = gmx_fjsp_set1_v2r8(fr->ic->sh_invrc6);
    rvdw             = gmx_fjsp_set1_v2r8(fr->rvdw);

    /* Avoid stupid compiler warnings */
    jnrA = jnrB = 0;
    j_coord_offsetA = 0;
    j_coord_offsetB = 0;

    outeriter        = 0;
    inneriter        = 0;

    /* Start outer loop over neighborlists */
    for(iidx=0; iidx<nri; iidx++)
    {
        /* Load shift vector for this list */
        i_shift_offset   = DIM*shiftidx[iidx];

        /* Load limits for loop over neighbors */
        j_index_start    = jindex[iidx];
        j_index_end      = jindex[iidx+1];

        /* Get outer coordinate index */
        inr              = iinr[iidx];
        i_coord_offset   = DIM*inr;

        /* Load i particle coords and add shift vector */
        gmx_fjsp_load_shift_and_1rvec_broadcast_v2r8(shiftvec+i_shift_offset,x+i_coord_offset,&ix0,&iy0,&iz0);

        fix0             = _fjsp_setzero_v2r8();
        fiy0             = _fjsp_setzero_v2r8();
        fiz0             = _fjsp_setzero_v2r8();

        /* Load parameters for i particles */
        vdwioffset0      = 2*nvdwtype*vdwtype[inr+0];

        /* Reset potential sums */
        vvdwsum          = _fjsp_setzero_v2r8();

        /* Start inner kernel loop */
        for(jidx=j_index_start; jidx<j_index_end-1; jidx+=2)
        {

            /* Get j neighbor index, and coordinate index */
            jnrA             = jjnr[jidx];
            jnrB             = jjnr[jidx+1];
            j_coord_offsetA  = DIM*jnrA;
            j_coord_offsetB  = DIM*jnrB;

            /* load j atom coordinates */
            gmx_fjsp_load_1rvec_2ptr_swizzle_v2r8(x+j_coord_offsetA,x+j_coord_offsetB,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _fjsp_sub_v2r8(ix0,jx0);
            dy00             = _fjsp_sub_v2r8(iy0,jy0);
            dz00             = _fjsp_sub_v2r8(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_fjsp_calc_rsq_v2r8(dx00,dy00,dz00);

            rinvsq00         = gmx_fjsp_inv_v2r8(rsq00);

            /* Load parameters for j particles */
            vdwjidx0A        = 2*vdwtype[jnrA+0];
            vdwjidx0B        = 2*vdwtype[jnrB+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            if (gmx_fjsp_any_lt_v2r8(rsq00,rcutoff2))
            {

            /* Compute parameters for interactions between i and j atoms */
            gmx_fjsp_load_2pair_swizzle_v2r8(vdwparam+vdwioffset0+vdwjidx0A,
                                         vdwparam+vdwioffset0+vdwjidx0B,&c6_00,&c12_00);

            /* LENNARD-JONES DISPERSION/REPULSION */

            rinvsix          = _fjsp_mul_v2r8(_fjsp_mul_v2r8(rinvsq00,rinvsq00),rinvsq00);
            vvdw6            = _fjsp_mul_v2r8(c6_00,rinvsix);
            vvdw12           = _fjsp_mul_v2r8(c12_00,_fjsp_mul_v2r8(rinvsix,rinvsix));
            vvdw             = _fjsp_msub_v2r8(_fjsp_nmsub_v2r8(c12_00,_fjsp_mul_v2r8(sh_vdw_invrcut6,sh_vdw_invrcut6),vvdw12),one_twelfth,
                                           _fjsp_mul_v2r8(_fjsp_nmsub_v2r8( c6_00,sh_vdw_invrcut6,vvdw6),one_sixth));
            fvdw             = _fjsp_mul_v2r8(_fjsp_sub_v2r8(vvdw12,vvdw6),rinvsq00);

            cutoff_mask      = _fjsp_cmplt_v2r8(rsq00,rcutoff2);

            /* Update potential sum for this i atom from the interaction with this j atom. */
            vvdw             = _fjsp_and_v2r8(vvdw,cutoff_mask);
            vvdwsum          = _fjsp_add_v2r8(vvdwsum,vvdw);

            fscal            = fvdw;

            fscal            = _fjsp_and_v2r8(fscal,cutoff_mask);

            /* Update vectorial force */
            fix0             = _fjsp_madd_v2r8(dx00,fscal,fix0);
            fiy0             = _fjsp_madd_v2r8(dy00,fscal,fiy0);
            fiz0             = _fjsp_madd_v2r8(dz00,fscal,fiz0);
            
            gmx_fjsp_decrement_fma_1rvec_2ptr_swizzle_v2r8(f+j_coord_offsetA,f+j_coord_offsetB,fscal,dx00,dy00,dz00);

            }

            /* Inner loop uses 44 flops */
        }

        if(jidx<j_index_end)
        {

            jnrA             = jjnr[jidx];
            j_coord_offsetA  = DIM*jnrA;

            /* load j atom coordinates */
            gmx_fjsp_load_1rvec_1ptr_swizzle_v2r8(x+j_coord_offsetA,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _fjsp_sub_v2r8(ix0,jx0);
            dy00             = _fjsp_sub_v2r8(iy0,jy0);
            dz00             = _fjsp_sub_v2r8(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_fjsp_calc_rsq_v2r8(dx00,dy00,dz00);

            rinvsq00         = gmx_fjsp_inv_v2r8(rsq00);

            /* Load parameters for j particles */
            vdwjidx0A        = 2*vdwtype[jnrA+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            if (gmx_fjsp_any_lt_v2r8(rsq00,rcutoff2))
            {

            /* Compute parameters for interactions between i and j atoms */
            gmx_fjsp_load_1pair_swizzle_v2r8(vdwparam+vdwioffset0+vdwjidx0A,&c6_00,&c12_00);

            /* LENNARD-JONES DISPERSION/REPULSION */

            rinvsix          = _fjsp_mul_v2r8(_fjsp_mul_v2r8(rinvsq00,rinvsq00),rinvsq00);
            vvdw6            = _fjsp_mul_v2r8(c6_00,rinvsix);
            vvdw12           = _fjsp_mul_v2r8(c12_00,_fjsp_mul_v2r8(rinvsix,rinvsix));
            vvdw             = _fjsp_msub_v2r8(_fjsp_nmsub_v2r8(c12_00,_fjsp_mul_v2r8(sh_vdw_invrcut6,sh_vdw_invrcut6),vvdw12),one_twelfth,
                                           _fjsp_mul_v2r8(_fjsp_nmsub_v2r8( c6_00,sh_vdw_invrcut6,vvdw6),one_sixth));
            fvdw             = _fjsp_mul_v2r8(_fjsp_sub_v2r8(vvdw12,vvdw6),rinvsq00);

            cutoff_mask      = _fjsp_cmplt_v2r8(rsq00,rcutoff2);

            /* Update potential sum for this i atom from the interaction with this j atom. */
            vvdw             = _fjsp_and_v2r8(vvdw,cutoff_mask);
            vvdw             = _fjsp_unpacklo_v2r8(vvdw,_fjsp_setzero_v2r8());
            vvdwsum          = _fjsp_add_v2r8(vvdwsum,vvdw);

            fscal            = fvdw;

            fscal            = _fjsp_and_v2r8(fscal,cutoff_mask);

            fscal            = _fjsp_unpacklo_v2r8(fscal,_fjsp_setzero_v2r8());

            /* Update vectorial force */
            fix0             = _fjsp_madd_v2r8(dx00,fscal,fix0);
            fiy0             = _fjsp_madd_v2r8(dy00,fscal,fiy0);
            fiz0             = _fjsp_madd_v2r8(dz00,fscal,fiz0);
            
            gmx_fjsp_decrement_fma_1rvec_1ptr_swizzle_v2r8(f+j_coord_offsetA,fscal,dx00,dy00,dz00);

            }

            /* Inner loop uses 44 flops */
        }

        /* End of innermost loop */

        gmx_fjsp_update_iforce_1atom_swizzle_v2r8(fix0,fiy0,fiz0,
                                              f+i_coord_offset,fshift+i_shift_offset);

        ggid                        = gid[iidx];
        /* Update potential energies */
        gmx_fjsp_update_1pot_v2r8(vvdwsum,kernel_data->energygrp_vdw+ggid);

        /* Increment number of inner iterations */
        inneriter                  += j_index_end - j_index_start;

        /* Outer loop uses 7 flops */
    }

    /* Increment number of outer iterations */
    outeriter        += nri;

    /* Update outer/inner flops */

    inc_nrnb(nrnb,eNR_NBKERNEL_VDW_VF,outeriter*7 + inneriter*44);
}
/*
 * Gromacs nonbonded kernel:   nb_kernel_ElecNone_VdwLJSh_GeomP1P1_F_sparc64_hpc_ace_double
 * Electrostatics interaction: None
 * VdW interaction:            LennardJones
 * Geometry:                   Particle-Particle
 * Calculate force/pot:        Force
 */
void
nb_kernel_ElecNone_VdwLJSh_GeomP1P1_F_sparc64_hpc_ace_double
                    (t_nblist                    * gmx_restrict       nlist,
                     rvec                        * gmx_restrict          xx,
                     rvec                        * gmx_restrict          ff,
                     t_forcerec                  * gmx_restrict          fr,
                     t_mdatoms                   * gmx_restrict     mdatoms,
                     nb_kernel_data_t gmx_unused * gmx_restrict kernel_data,
                     t_nrnb                      * gmx_restrict        nrnb)
{
    /* Suffixes 0,1,2,3 refer to particle indices for waters in the inner or outer loop, or
     * just 0 for non-waters.
     * Suffixes A,B refer to j loop unrolling done with double precision SIMD, e.g. for the two different
     * jnr indices corresponding to data put in the four positions in the SIMD register.
     */
    int              i_shift_offset,i_coord_offset,outeriter,inneriter;
    int              j_index_start,j_index_end,jidx,nri,inr,ggid,iidx;
    int              jnrA,jnrB;
    int              j_coord_offsetA,j_coord_offsetB;
    int              *iinr,*jindex,*jjnr,*shiftidx,*gid;
    real             rcutoff_scalar;
    real             *shiftvec,*fshift,*x,*f;
    _fjsp_v2r8       tx,ty,tz,fscal,rcutoff,rcutoff2,jidxall;
    int              vdwioffset0;
    _fjsp_v2r8       ix0,iy0,iz0,fix0,fiy0,fiz0,iq0,isai0;
    int              vdwjidx0A,vdwjidx0B;
    _fjsp_v2r8       jx0,jy0,jz0,fjx0,fjy0,fjz0,jq0,isaj0;
    _fjsp_v2r8       dx00,dy00,dz00,rsq00,rinv00,rinvsq00,r00,qq00,c6_00,c12_00;
    int              nvdwtype;
    _fjsp_v2r8       rinvsix,rvdw,vvdw,vvdw6,vvdw12,fvdw,fvdw6,fvdw12,vvdwsum,sh_vdw_invrcut6;
    int              *vdwtype;
    real             *vdwparam;
    _fjsp_v2r8       one_sixth   = gmx_fjsp_set1_v2r8(1.0/6.0);
    _fjsp_v2r8       one_twelfth = gmx_fjsp_set1_v2r8(1.0/12.0);
    _fjsp_v2r8       itab_tmp;
    _fjsp_v2r8       dummy_mask,cutoff_mask;
    _fjsp_v2r8       one     = gmx_fjsp_set1_v2r8(1.0);
    _fjsp_v2r8       two     = gmx_fjsp_set1_v2r8(2.0);
    union { _fjsp_v2r8 simd; long long int i[2]; } vfconv,gbconv,ewconv;

    x                = xx[0];
    f                = ff[0];

    nri              = nlist->nri;
    iinr             = nlist->iinr;
    jindex           = nlist->jindex;
    jjnr             = nlist->jjnr;
    shiftidx         = nlist->shift;
    gid              = nlist->gid;
    shiftvec         = fr->shift_vec[0];
    fshift           = fr->fshift[0];
    nvdwtype         = fr->ntype;
    vdwparam         = fr->nbfp;
    vdwtype          = mdatoms->typeA;

    rcutoff_scalar   = fr->rvdw;
    rcutoff          = gmx_fjsp_set1_v2r8(rcutoff_scalar);
    rcutoff2         = _fjsp_mul_v2r8(rcutoff,rcutoff);

    sh_vdw_invrcut6  = gmx_fjsp_set1_v2r8(fr->ic->sh_invrc6);
    rvdw             = gmx_fjsp_set1_v2r8(fr->rvdw);

    /* Avoid stupid compiler warnings */
    jnrA = jnrB = 0;
    j_coord_offsetA = 0;
    j_coord_offsetB = 0;

    outeriter        = 0;
    inneriter        = 0;

    /* Start outer loop over neighborlists */
    for(iidx=0; iidx<nri; iidx++)
    {
        /* Load shift vector for this list */
        i_shift_offset   = DIM*shiftidx[iidx];

        /* Load limits for loop over neighbors */
        j_index_start    = jindex[iidx];
        j_index_end      = jindex[iidx+1];

        /* Get outer coordinate index */
        inr              = iinr[iidx];
        i_coord_offset   = DIM*inr;

        /* Load i particle coords and add shift vector */
        gmx_fjsp_load_shift_and_1rvec_broadcast_v2r8(shiftvec+i_shift_offset,x+i_coord_offset,&ix0,&iy0,&iz0);

        fix0             = _fjsp_setzero_v2r8();
        fiy0             = _fjsp_setzero_v2r8();
        fiz0             = _fjsp_setzero_v2r8();

        /* Load parameters for i particles */
        vdwioffset0      = 2*nvdwtype*vdwtype[inr+0];

        /* Start inner kernel loop */
        for(jidx=j_index_start; jidx<j_index_end-1; jidx+=2)
        {

            /* Get j neighbor index, and coordinate index */
            jnrA             = jjnr[jidx];
            jnrB             = jjnr[jidx+1];
            j_coord_offsetA  = DIM*jnrA;
            j_coord_offsetB  = DIM*jnrB;

            /* load j atom coordinates */
            gmx_fjsp_load_1rvec_2ptr_swizzle_v2r8(x+j_coord_offsetA,x+j_coord_offsetB,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _fjsp_sub_v2r8(ix0,jx0);
            dy00             = _fjsp_sub_v2r8(iy0,jy0);
            dz00             = _fjsp_sub_v2r8(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_fjsp_calc_rsq_v2r8(dx00,dy00,dz00);

            rinvsq00         = gmx_fjsp_inv_v2r8(rsq00);

            /* Load parameters for j particles */
            vdwjidx0A        = 2*vdwtype[jnrA+0];
            vdwjidx0B        = 2*vdwtype[jnrB+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            if (gmx_fjsp_any_lt_v2r8(rsq00,rcutoff2))
            {

            /* Compute parameters for interactions between i and j atoms */
            gmx_fjsp_load_2pair_swizzle_v2r8(vdwparam+vdwioffset0+vdwjidx0A,
                                         vdwparam+vdwioffset0+vdwjidx0B,&c6_00,&c12_00);

            /* LENNARD-JONES DISPERSION/REPULSION */

            rinvsix          = _fjsp_mul_v2r8(_fjsp_mul_v2r8(rinvsq00,rinvsq00),rinvsq00);
            fvdw             = _fjsp_mul_v2r8(_fjsp_msub_v2r8(c12_00,rinvsix,c6_00),_fjsp_mul_v2r8(rinvsix,rinvsq00));

            cutoff_mask      = _fjsp_cmplt_v2r8(rsq00,rcutoff2);

            fscal            = fvdw;

            fscal            = _fjsp_and_v2r8(fscal,cutoff_mask);

            /* Update vectorial force */
            fix0             = _fjsp_madd_v2r8(dx00,fscal,fix0);
            fiy0             = _fjsp_madd_v2r8(dy00,fscal,fiy0);
            fiz0             = _fjsp_madd_v2r8(dz00,fscal,fiz0);
            
            gmx_fjsp_decrement_fma_1rvec_2ptr_swizzle_v2r8(f+j_coord_offsetA,f+j_coord_offsetB,fscal,dx00,dy00,dz00);

            }

            /* Inner loop uses 33 flops */
        }

        if(jidx<j_index_end)
        {

            jnrA             = jjnr[jidx];
            j_coord_offsetA  = DIM*jnrA;

            /* load j atom coordinates */
            gmx_fjsp_load_1rvec_1ptr_swizzle_v2r8(x+j_coord_offsetA,
                                              &jx0,&jy0,&jz0);

            /* Calculate displacement vector */
            dx00             = _fjsp_sub_v2r8(ix0,jx0);
            dy00             = _fjsp_sub_v2r8(iy0,jy0);
            dz00             = _fjsp_sub_v2r8(iz0,jz0);

            /* Calculate squared distance and things based on it */
            rsq00            = gmx_fjsp_calc_rsq_v2r8(dx00,dy00,dz00);

            rinvsq00         = gmx_fjsp_inv_v2r8(rsq00);

            /* Load parameters for j particles */
            vdwjidx0A        = 2*vdwtype[jnrA+0];

            /**************************
             * CALCULATE INTERACTIONS *
             **************************/

            if (gmx_fjsp_any_lt_v2r8(rsq00,rcutoff2))
            {

            /* Compute parameters for interactions between i and j atoms */
            gmx_fjsp_load_1pair_swizzle_v2r8(vdwparam+vdwioffset0+vdwjidx0A,&c6_00,&c12_00);

            /* LENNARD-JONES DISPERSION/REPULSION */

            rinvsix          = _fjsp_mul_v2r8(_fjsp_mul_v2r8(rinvsq00,rinvsq00),rinvsq00);
            fvdw             = _fjsp_mul_v2r8(_fjsp_msub_v2r8(c12_00,rinvsix,c6_00),_fjsp_mul_v2r8(rinvsix,rinvsq00));

            cutoff_mask      = _fjsp_cmplt_v2r8(rsq00,rcutoff2);

            fscal            = fvdw;

            fscal            = _fjsp_and_v2r8(fscal,cutoff_mask);

            fscal            = _fjsp_unpacklo_v2r8(fscal,_fjsp_setzero_v2r8());

            /* Update vectorial force */
            fix0             = _fjsp_madd_v2r8(dx00,fscal,fix0);
            fiy0             = _fjsp_madd_v2r8(dy00,fscal,fiy0);
            fiz0             = _fjsp_madd_v2r8(dz00,fscal,fiz0);
            
            gmx_fjsp_decrement_fma_1rvec_1ptr_swizzle_v2r8(f+j_coord_offsetA,fscal,dx00,dy00,dz00);

            }

            /* Inner loop uses 33 flops */
        }

        /* End of innermost loop */

        gmx_fjsp_update_iforce_1atom_swizzle_v2r8(fix0,fiy0,fiz0,
                                              f+i_coord_offset,fshift+i_shift_offset);

        /* Increment number of inner iterations */
        inneriter                  += j_index_end - j_index_start;

        /* Outer loop uses 6 flops */
    }

    /* Increment number of outer iterations */
    outeriter        += nri;

    /* Update outer/inner flops */

    inc_nrnb(nrnb,eNR_NBKERNEL_VDW_F,outeriter*6 + inneriter*33);
}
