/*
 * This file is part of the GROMACS molecular simulation package.
 *
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2004, The GROMACS development team.
 * Copyright (c) 2013,2014,2015,2016,2017,2018,2019, by the GROMACS development team, led by
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
#include "gmxpre.h"

#include "qmmm.h"

#include "config.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>

#include "gromacs/domdec/domdec_struct.h"
#include "gromacs/fileio/confio.h"
#include "gromacs/gmxlib/network.h"
#include "gromacs/gmxlib/nrnb.h"
#include "gromacs/math/functions.h"
#include "gromacs/math/units.h"
#include "gromacs/math/vec.h"
#include "gromacs/mdlib/qm_gamess.h"
#include "gromacs/mdlib/qm_gaussian.h"
#include "gromacs/mdlib/qm_mopac.h"
#include "gromacs/mdlib/qm_orca.h"
#include "gromacs/mdtypes/commrec.h"
#include "gromacs/mdtypes/forceoutput.h"
#include "gromacs/mdtypes/forcerec.h"
#include "gromacs/mdtypes/inputrec.h"
#include "gromacs/mdtypes/md_enums.h"
#include "gromacs/mdtypes/mdatom.h"
#include "gromacs/mdtypes/nblist.h"
#include "gromacs/pbcutil/ishift.h"
#include "gromacs/pbcutil/pbc.h"
#include "gromacs/topology/mtop_lookup.h"
#include "gromacs/topology/mtop_util.h"
#include "gromacs/topology/topology.h"
#include "gromacs/utility/fatalerror.h"
#include "gromacs/utility/smalloc.h"

// When not built in a configuration with QMMM support, much of this
// code is unreachable by design. Tell clang not to warn about it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunreachable-code"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"

/* this struct and these comparison functions are needed for creating
 * a QMMM input for the QM routines from the QMMM neighbor list.
 */

typedef struct
{
    int j;
    int shift;
} t_j_particle;

static bool struct_comp(const t_j_particle& a, const t_j_particle& b)
{
    return a.j < b.j;
}

static real call_QMroutine(const t_commrec gmx_unused* cr,
                           const t_QMMMrec gmx_unused* qmmm,
                           t_QMrec gmx_unused* qm,
                           t_MMrec gmx_unused* mm,
                           rvec gmx_unused f[],
                           rvec gmx_unused fshift[])
{
    /* makes a call to the requested QM routine (qm->QMmethod)
     * Note that f is actually the gradient, i.e. -f
     */
    /* do a semi-empiprical calculation */

    if (qm->QMmethod < eQMmethodRHF && !(mm->nrMMatoms))
    {
        if (GMX_QMMM_MOPAC)
        {
            if (qm->bSH)
            {
                return call_mopac_SH(qm, mm, f, fshift);
            }
            else
            {
                return call_mopac(qm, mm, f, fshift);
            }
        }
        else
        {
            gmx_fatal(FARGS, "Semi-empirical QM only supported with Mopac.");
        }
    }
    else
    {
        /* do an ab-initio calculation */
        if (qm->bSH && qm->QMmethod == eQMmethodCASSCF)
        {
            if (GMX_QMMM_GAUSSIAN)
            {
                return call_gaussian_SH(qmmm, qm, mm, f, fshift);
            }
            else
            {
                gmx_fatal(FARGS, "Ab-initio Surface-hopping only supported with Gaussian.");
            }
        }
        else
        {
            if (GMX_QMMM_GAMESS)
            {
                return call_gamess(qm, mm, f, fshift);
            }
            else if (GMX_QMMM_GAUSSIAN)
            {
                return call_gaussian(qmmm, qm, mm, f, fshift);
            }
            else if (GMX_QMMM_ORCA)
            {
                return call_orca(qmmm, qm, mm, f, fshift);
            }
            else
            {
                gmx_fatal(FARGS,
                          "Ab-initio calculation only supported with Gamess, Gaussian or ORCA.");
            }
        }
    }
}

static void init_QMroutine(const t_commrec gmx_unused* cr, t_QMrec gmx_unused* qm, t_MMrec gmx_unused* mm)
{
    /* makes a call to the requested QM routine (qm->QMmethod)
     */
    if (qm->QMmethod < eQMmethodRHF)
    {
        if (GMX_QMMM_MOPAC)
        {
            /* do a semi-empiprical calculation */
            init_mopac(qm);
        }
        else
        {
            gmx_fatal(FARGS, "Semi-empirical QM only supported with Mopac.");
        }
    }
    else
    {
        /* do an ab-initio calculation */
        if (GMX_QMMM_GAMESS)
        {
            init_gamess(cr, qm, mm);
        }
        else if (GMX_QMMM_GAUSSIAN)
        {
            init_gaussian(qm);
        }
        else if (GMX_QMMM_ORCA)
        {
            init_orca(qm);
        }
        else
        {
            gmx_fatal(FARGS, "Ab-initio calculation only supported with Gamess, Gaussian or ORCA.");
        }
    }
} /* init_QMroutine */

static void update_QMMM_coord(const rvec* x, const t_forcerec* fr, t_QMrec* qm, t_MMrec* mm)
{
    /* shifts the QM and MM particles into the central box and stores
     * these shifted coordinates in the coordinate arrays of the
     * QMMMrec. These coordinates are passed on the QM subroutines.
     */
    int i;

    /* shift the QM atoms into the central box
     */
    for (i = 0; i < qm->nrQMatoms; i++)
    {
        rvec_sub(x[qm->indexQM[i]], fr->shift_vec[qm->shiftQM[i]], qm->xQM[i]);
    }
    /* also shift the MM atoms into the central box, if any
     */
    for (i = 0; i < mm->nrMMatoms; i++)
    {
        rvec_sub(x[mm->indexMM[i]], fr->shift_vec[mm->shiftMM[i]], mm->xMM[i]);
    }
} /* update_QMMM_coord */

/* end of QMMM subroutines */

/* QMMM core routines */

static t_QMrec* mk_QMrec()
{
    t_QMrec* qm;
    snew(qm, 1);
    return qm;
} /* mk_QMrec */

static t_MMrec* mk_MMrec()
{
    t_MMrec* mm;
    snew(mm, 1);
    return mm;
} /* mk_MMrec */

static void init_QMrec(int grpnr, t_QMrec* qm, int nr, const int* atomarray, const gmx_mtop_t* mtop, const t_inputrec* ir)
{
    /* fills the t_QMrec struct of QM group grpnr
     */

    qm->nrQMatoms = nr;
    snew(qm->xQM, nr);
    snew(qm->indexQM, nr);
    snew(qm->shiftQM, nr); /* the shifts */
    for (int i = 0; i < nr; i++)
    {
        qm->indexQM[i] = atomarray[i];
    }

    snew(qm->atomicnumberQM, nr);
    int molb = 0;
    for (int i = 0; i < qm->nrQMatoms; i++)
    {
        const t_atom& atom = mtopGetAtomParameters(mtop, qm->indexQM[i], &molb);
        qm->nelectrons += mtop->atomtypes.atomnumber[atom.type];
        qm->atomicnumberQM[i] = mtop->atomtypes.atomnumber[atom.type];
    }

    qm->QMcharge     = ir->opts.QMcharge[grpnr];
    qm->multiplicity = ir->opts.QMmult[grpnr];
    qm->nelectrons -= ir->opts.QMcharge[grpnr];

    qm->QMmethod = ir->opts.QMmethod[grpnr];
    qm->QMbasis  = ir->opts.QMbasis[grpnr];
    /* trajectory surface hopping setup (Gaussian only) */
    qm->bSH          = ir->opts.bSH[grpnr];
    qm->CASorbitals  = ir->opts.CASorbitals[grpnr];
    qm->CASelectrons = ir->opts.CASelectrons[grpnr];
    qm->SAsteps      = ir->opts.SAsteps[grpnr];
    qm->SAon         = ir->opts.SAon[grpnr];
    qm->SAoff        = ir->opts.SAoff[grpnr];
    /* hack to prevent gaussian from reinitializing all the time */
    qm->nQMcpus = 0; /* number of CPU's to be used by g01, is set
                      * upon initializing gaussian
                      * (init_gaussian()
                      */
    /* print the current layer to allow users to check their input */
    fprintf(stderr, "Layer %d\nnr of QM atoms %d\n", grpnr, nr);
    fprintf(stderr, "QMlevel: %s/%s\n\n", eQMmethod_names[qm->QMmethod], eQMbasis_names[qm->QMbasis]);
} /* init_QMrec */

static t_QMrec* copy_QMrec(t_QMrec* qm)
{
    /* copies the contents of qm into a new t_QMrec struct */
    t_QMrec* qmcopy;
    int      i;

    qmcopy            = mk_QMrec();
    qmcopy->nrQMatoms = qm->nrQMatoms;
    snew(qmcopy->xQM, qmcopy->nrQMatoms);
    snew(qmcopy->indexQM, qmcopy->nrQMatoms);
    snew(qmcopy->atomicnumberQM, qm->nrQMatoms);
    snew(qmcopy->shiftQM, qmcopy->nrQMatoms); /* the shifts */
    for (i = 0; i < qmcopy->nrQMatoms; i++)
    {
        qmcopy->shiftQM[i]        = qm->shiftQM[i];
        qmcopy->indexQM[i]        = qm->indexQM[i];
        qmcopy->atomicnumberQM[i] = qm->atomicnumberQM[i];
    }
    qmcopy->nelectrons   = qm->nelectrons;
    qmcopy->multiplicity = qm->multiplicity;
    qmcopy->QMcharge     = qm->QMcharge;
    qmcopy->nelectrons   = qm->nelectrons;
    qmcopy->QMmethod     = qm->QMmethod;
    qmcopy->QMbasis      = qm->QMbasis;
    /* trajectory surface hopping setup (Gaussian only) */
    qmcopy->bSH          = qm->bSH;
    qmcopy->CASorbitals  = qm->CASorbitals;
    qmcopy->CASelectrons = qm->CASelectrons;
    qmcopy->SAsteps      = qm->SAsteps;
    qmcopy->SAon         = qm->SAon;
    qmcopy->SAoff        = qm->SAoff;

    /* Gaussian init. variables */
    qmcopy->nQMcpus = qm->nQMcpus;
    for (i = 0; i < DIM; i++)
    {
        qmcopy->SHbasis[i] = qm->SHbasis[i];
    }
    qmcopy->QMmem    = qm->QMmem;
    qmcopy->accuracy = qm->accuracy;
    qmcopy->cpmcscf  = qm->cpmcscf;
    qmcopy->SAstep   = qm->SAstep;

    return (qmcopy);

} /*copy_QMrec */

#if GMX_QMMM

t_QMMMrec* mk_QMMMrec()
{
    t_QMMMrec* qr;

    snew(qr, 1);

    return qr;

} /* mk_QMMMrec */

#else /* GMX_QMMM */

t_QMMMrec* mk_QMMMrec()
{
    gmx_incons("Compiled without QMMM");
} /* mk_QMMMrec */
#endif

std::vector<int> qmmmAtomIndices(const t_inputrec& ir, const gmx_mtop_t& mtop)
{
    const int               numQmmmGroups = ir.opts.ngQM;
    const SimulationGroups& groups        = mtop.groups;
    std::vector<int>        qmmmAtoms;
    for (int i = 0; i < numQmmmGroups; i++)
    {
        for (const AtomProxy atomP : AtomRange(mtop))
        {
            int index = atomP.globalAtomNumber();
            if (getGroupType(groups, SimulationAtomGroupType::QuantumMechanics, index) == i)
            {
                qmmmAtoms.push_back(index);
            }
        }
        if (ir.QMMMscheme == eQMMMschemeoniom)
        {
            /* I assume that users specify the QM groups from small to
             * big(ger) in the mdp file
             */
            gmx_mtop_ilistloop_all_t iloop      = gmx_mtop_ilistloop_all_init(&mtop);
            int                      nral1      = 1 + NRAL(F_VSITE2);
            int                      atomOffset = 0;
            while (const InteractionLists* ilists = gmx_mtop_ilistloop_all_next(iloop, &atomOffset))
            {
                const InteractionList& ilist = (*ilists)[F_VSITE2];
                for (int j = 0; j < ilist.size(); j += nral1)
                {
                    const int vsite = atomOffset + ilist.iatoms[j];     /* the vsite         */
                    const int ai    = atomOffset + ilist.iatoms[j + 1]; /* constructing atom */
                    const int aj    = atomOffset + ilist.iatoms[j + 2]; /* constructing atom */
                    if (getGroupType(groups, SimulationAtomGroupType::QuantumMechanics, vsite)
                                == getGroupType(groups, SimulationAtomGroupType::QuantumMechanics, ai)
                        && getGroupType(groups, SimulationAtomGroupType::QuantumMechanics, vsite)
                                   == getGroupType(groups, SimulationAtomGroupType::QuantumMechanics, aj))
                    {
                        /* this dummy link atom needs to be removed from qmmmAtoms
                         * before making the QMrec of this layer!
                         */
                        qmmmAtoms.erase(std::remove_if(qmmmAtoms.begin(), qmmmAtoms.end(),
                                                       [&vsite](int atom) { return atom == vsite; }),
                                        qmmmAtoms.end());
                    }
                }
            }
        }
    }
    return qmmmAtoms;
}

void removeQmmmAtomCharges(gmx_mtop_t* mtop, gmx::ArrayRef<const int> qmmmAtoms)
{
    int molb = 0;
    for (gmx::index i = 0; i < qmmmAtoms.ssize(); i++)
    {
        int indexInMolecule;
        mtopGetMolblockIndex(mtop, qmmmAtoms[i], &molb, nullptr, &indexInMolecule);
        t_atom* atom = &mtop->moltype[mtop->molblock[molb].type].atoms.atom[indexInMolecule];
        atom->q      = 0.0;
        atom->qB     = 0.0;
    }
}

void init_QMMMrec(const t_commrec* cr, const gmx_mtop_t* mtop, const t_inputrec* ir, const t_forcerec* fr)
{
    /* we put the atomsnumbers of atoms that belong to the QMMM group in
     * an array that will be copied later to QMMMrec->indexQM[..]. Also
     * it will be used to create an QMMMrec->bQMMM index array that
     * simply contains true/false for QM and MM (the other) atoms.
     */

    t_QMMMrec* qr;
    t_MMrec*   mm;

    if (!GMX_QMMM)
    {
        gmx_incons("Compiled without QMMM");
    }

    if (ir->cutoff_scheme != ecutsGROUP)
    {
        gmx_fatal(FARGS, "QMMM is currently only supported with cutoff-scheme=group");
    }
    if (!EI_DYNAMICS(ir->eI))
    {
        gmx_fatal(FARGS, "QMMM is only supported with dynamics");
    }

    /* issue a fatal if the user wants to run with more than one node */
    if (PAR(cr))
    {
        gmx_fatal(FARGS, "QM/MM does not work in parallel, use a single rank instead\n");
    }

    /* Make a local copy of the QMMMrec */
    qr = fr->qr;

    /* bQMMM[..] is an array containing TRUE/FALSE for atoms that are
     * QM/not QM. We first set all elemenst at false. Afterwards we use
     * the qm_arr (=MMrec->indexQM) to changes the elements
     * corresponding to the QM atoms at TRUE.  */

    qr->QMMMscheme = ir->QMMMscheme;

    /* we take the possibility into account that a user has
     * defined more than one QM group:
     */
    /* an ugly work-around in case there is only one group In this case
     * the whole system is treated as QM. Otherwise the second group is
     * always the rest of the total system and is treated as MM.
     */

    /* small problem if there is only QM.... so no MM */

    int numQmmmGroups = ir->opts.ngQM;

    if (qr->QMMMscheme == eQMMMschemeoniom)
    {
        qr->nrQMlayers = numQmmmGroups;
    }
    else
    {
        qr->nrQMlayers = 1;
    }

    /* there are numQmmmGroups groups of QM atoms. In case of multiple QM groups
     * I assume that the users wants to do ONIOM. However, maybe it
     * should also be possible to define more than one QM subsystem with
     * independent neighbourlists. I have to think about
     * that.. 11-11-2003
     */
    std::vector<int> qmmmAtoms = qmmmAtomIndices(*ir, *mtop);
    snew(qr->qm, numQmmmGroups);
    for (int i = 0; i < numQmmmGroups; i++)
    {
        /* new layer */
        if (qr->QMMMscheme == eQMMMschemeoniom)
        {
            /* add the atoms to the bQMMM array
             */

            /* I assume that users specify the QM groups from small to
             * big(ger) in the mdp file
             */
            qr->qm[i] = mk_QMrec();
            /* store QM atoms in this layer in the QMrec and initialise layer
             */
            init_QMrec(i, qr->qm[i], qmmmAtoms.size(), qmmmAtoms.data(), mtop, ir);
        }
    }
    if (qr->QMMMscheme != eQMMMschemeoniom)
    {

        /* standard QMMM, all layers are merged together so there is one QM
         * subsystem and one MM subsystem.
         * Also we set the charges to zero in mtop to prevent the innerloops
         * from doubly counting the electostatic QM MM interaction
         * TODO: Consider doing this in grompp instead.
         */

        qr->qm[0] = mk_QMrec();
        /* store QM atoms in the QMrec and initialise
         */
        init_QMrec(0, qr->qm[0], qmmmAtoms.size(), qmmmAtoms.data(), mtop, ir);

        /* MM rec creation */
        mm              = mk_MMrec();
        mm->scalefactor = ir->scalefactor;
        mm->nrMMatoms   = (mtop->natoms) - (qr->qm[0]->nrQMatoms); /* rest of the atoms */
        qr->mm          = mm;
    }
    else /* ONIOM */
    {    /* MM rec creation */
        mm              = mk_MMrec();
        mm->scalefactor = ir->scalefactor;
        mm->nrMMatoms   = 0;
        qr->mm          = mm;
    }

    /* these variables get updated in the update QMMMrec */

    if (qr->nrQMlayers == 1)
    {
        /* with only one layer there is only one initialisation
         * needed. Multilayer is a bit more complicated as it requires
         * re-initialisation at every step of the simulation. This is due
         * to the use of COMMON blocks in the fortran QM subroutines.
         */
        if (qr->qm[0]->QMmethod < eQMmethodRHF)
        {
            if (GMX_QMMM_MOPAC)
            {
                /* semi-empiprical 1-layer ONIOM calculation requested (mopac93) */
                init_mopac(qr->qm[0]);
            }
            else
            {
                gmx_fatal(FARGS, "Semi-empirical QM only supported with Mopac.");
            }
        }
        else
        {
            /* ab initio calculation requested (gamess/gaussian/ORCA) */
            if (GMX_QMMM_GAMESS)
            {
                init_gamess(cr, qr->qm[0], qr->mm);
            }
            else if (GMX_QMMM_GAUSSIAN)
            {
                init_gaussian(qr->qm[0]);
            }
            else if (GMX_QMMM_ORCA)
            {
                init_orca(qr->qm[0]);
            }
            else
            {
                gmx_fatal(FARGS,
                          "Ab-initio calculation only supported with Gamess, Gaussian or ORCA.");
            }
        }
    }
} /* init_QMMMrec */

void update_QMMMrec(const t_commrec* cr, const t_forcerec* fr, const rvec* x, const t_mdatoms* md, const matrix box)
{
    /* updates the coordinates of both QM atoms and MM atoms and stores
     * them in the QMMMrec.
     *
     * NOTE: is NOT yet working if there are no PBC. Also in ns.c, simple
     * ns needs to be fixed!
     */
    int           mm_max = 0, mm_nr = 0, mm_nr_new, i, j, is, k, shift;
    t_j_particle *mm_j_particles = nullptr, *qm_i_particles = nullptr;
    t_QMMMrec*    qr;
    t_nblist*     QMMMlist;
    rvec          dx;
    ivec          crd;
    t_QMrec*      qm;
    t_MMrec*      mm;
    t_pbc         pbc;
    int*          parallelMMarray = nullptr;

    if (!GMX_QMMM)
    {
        gmx_incons("Compiled without QMMM");
    }

    /* every cpu has this array. On every processor we fill this array
     * with 1's and 0's. 1's indicate the atoms is a QM atom on the
     * current cpu in a later stage these arrays are all summed. indexes
     * > 0 indicate the atom is a QM atom. Every node therefore knows
     * whcih atoms are part of the QM subsystem.
     */
    /* copy some pointers */
    qr       = fr->qr;
    mm       = qr->mm;
    QMMMlist = fr->QMMMlist;

    /*  init_pbc(box);  needs to be called first, see pbc.h */
    ivec null_ivec;
    clear_ivec(null_ivec);
    set_pbc_dd(&pbc, fr->ePBC, DOMAINDECOMP(cr) ? cr->dd->nc : null_ivec, FALSE, box);
    /* only in standard (normal) QMMM we need the neighbouring MM
     * particles to provide a electric field of point charges for the QM
     * atoms.
     */
    if (qr->QMMMscheme == eQMMMschemenormal) /* also implies 1 QM-layer */
    {
        /* we NOW create/update a number of QMMMrec entries:
         *
         * 1) the shiftQM, containing the shifts of the QM atoms
         *
         * 2) the indexMM array, containing the index of the MM atoms
         *
         * 3) the shiftMM, containing the shifts of the MM atoms
         *
         * 4) the shifted coordinates of the MM atoms
         *
         * the shifts are used for computing virial of the QM/MM particles.
         */
        qm = qr->qm[0]; /* in case of normal QMMM, there is only one group */
        snew(qm_i_particles, QMMMlist->nri);
        if (QMMMlist->nri)
        {
            qm_i_particles[0].shift = XYZ2IS(0, 0, 0);
            for (i = 0; i < QMMMlist->nri; i++)
            {
                qm_i_particles[i].j = QMMMlist->iinr[i];

                if (i)
                {
                    qm_i_particles[i].shift =
                            pbc_dx_aiuc(&pbc, x[QMMMlist->iinr[0]], x[QMMMlist->iinr[i]], dx);
                }
                /* However, since nri >= nrQMatoms, we do a quicksort, and throw
                 * out double, triple, etc. entries later, as we do for the MM
                 * list too.
                 */

                /* compute the shift for the MM j-particles with respect to
                 * the QM i-particle and store them.
                 */

                crd[0] = IS2X(QMMMlist->shift[i]) + IS2X(qm_i_particles[i].shift);
                crd[1] = IS2Y(QMMMlist->shift[i]) + IS2Y(qm_i_particles[i].shift);
                crd[2] = IS2Z(QMMMlist->shift[i]) + IS2Z(qm_i_particles[i].shift);
                is     = XYZ2IS(crd[0], crd[1], crd[2]);
                for (j = QMMMlist->jindex[i]; j < QMMMlist->jindex[i + 1]; j++)
                {
                    if (mm_nr >= mm_max)
                    {
                        mm_max += 1000;
                        srenew(mm_j_particles, mm_max);
                    }

                    mm_j_particles[mm_nr].j     = QMMMlist->jjnr[j];
                    mm_j_particles[mm_nr].shift = is;
                    mm_nr++;
                }
            }

            /* quicksort QM and MM shift arrays and throw away multiple entries */


            std::sort(qm_i_particles, qm_i_particles + QMMMlist->nri, struct_comp);
            /* The mm_j_particles argument to qsort is not allowed to be nullptr */
            if (mm_nr > 0)
            {
                std::sort(mm_j_particles, mm_j_particles + mm_nr, struct_comp);
            }
            /* remove multiples in the QM shift array, since in init_QMMM() we
             * went through the atom numbers from 0 to md.nr, the order sorted
             * here matches the one of QMindex already.
             */
            j = 0;
            for (i = 0; i < QMMMlist->nri; i++)
            {
                if (i == 0 || qm_i_particles[i].j != qm_i_particles[i - 1].j)
                {
                    qm_i_particles[j++] = qm_i_particles[i];
                }
            }
            mm_nr_new = 0;
            /* Remove double entries for the MM array.
             * Also remove mm atoms that have no charges!
             * actually this is already done in the ns.c
             */
            for (i = 0; i < mm_nr; i++)
            {
                if ((i == 0 || mm_j_particles[i].j != mm_j_particles[i - 1].j)
                    && !md->bQM[mm_j_particles[i].j]
                    && ((md->chargeA[mm_j_particles[i].j] != 0.0_real)
                        || (md->chargeB && (md->chargeB[mm_j_particles[i].j] != 0.0_real))))
                {
                    mm_j_particles[mm_nr_new++] = mm_j_particles[i];
                }
            }
            mm_nr = mm_nr_new;
            /* store the data retrieved above into the QMMMrec
             */
            k = 0;
            /* Keep the compiler happy,
             * shift will always be set in the loop for i=0
             */
            shift = 0;
            for (i = 0; i < qm->nrQMatoms; i++)
            {
                /* not all qm particles might have appeared as i
                 * particles. They might have been part of the same charge
                 * group for instance.
                 */
                if (qm->indexQM[i] == qm_i_particles[k].j)
                {
                    shift = qm_i_particles[k++].shift;
                }
                /* use previous shift, assuming they belong the same charge
                 * group anyway,
                 */

                qm->shiftQM[i] = shift;
            }
        }
        /* parallel excecution */
        if (PAR(cr))
        {
            snew(parallelMMarray, 2 * (md->nr));
            /* only MM particles have a 1 at their atomnumber. The second part
             * of the array contains the shifts. Thus:
             * p[i]=1/0 depending on wether atomnumber i is a MM particle in the QM
             * step or not. p[i+md->nr] is the shift of atomnumber i.
             */
            for (i = 0; i < 2 * (md->nr); i++)
            {
                parallelMMarray[i] = 0;
            }

            for (i = 0; i < mm_nr; i++)
            {
                parallelMMarray[mm_j_particles[i].j]            = 1;
                parallelMMarray[mm_j_particles[i].j + (md->nr)] = mm_j_particles[i].shift;
            }
            gmx_sumi(md->nr, parallelMMarray, cr);
            mm_nr = 0;

            mm_max = 0;
            for (i = 0; i < md->nr; i++)
            {
                if (parallelMMarray[i])
                {
                    if (mm_nr >= mm_max)
                    {
                        mm_max += 1000;
                        srenew(mm->indexMM, mm_max);
                        srenew(mm->shiftMM, mm_max);
                    }
                    mm->indexMM[mm_nr]   = i;
                    mm->shiftMM[mm_nr++] = parallelMMarray[i + md->nr] / parallelMMarray[i];
                }
            }
            mm->nrMMatoms = mm_nr;
            free(parallelMMarray);
        }
        /* serial execution */
        else
        {
            mm->nrMMatoms = mm_nr;
            srenew(mm->shiftMM, mm_nr);
            srenew(mm->indexMM, mm_nr);
            for (i = 0; i < mm_nr; i++)
            {
                mm->indexMM[i] = mm_j_particles[i].j;
                mm->shiftMM[i] = mm_j_particles[i].shift;
            }
        }
        /* (re) allocate memory for the MM coordiate array. The QM
         * coordinate array was already allocated in init_QMMM, and is
         * only (re)filled in the update_QMMM_coordinates routine
         */
        srenew(mm->xMM, mm->nrMMatoms);
        /* now we (re) fill the array that contains the MM charges with
         * the forcefield charges. If requested, these charges will be
         * scaled by a factor
         */
        srenew(mm->MMcharges, mm->nrMMatoms);
        for (i = 0; i < mm->nrMMatoms; i++) /* no free energy yet */
        {
            mm->MMcharges[i] = md->chargeA[mm->indexMM[i]] * mm->scalefactor;
        }
        /* the next routine fills the coordinate fields in the QMMM rec of
         * both the qunatum atoms and the MM atoms, using the shifts
         * calculated above.
         */

        update_QMMM_coord(x, fr, qr->qm[0], qr->mm);
        free(qm_i_particles);
        free(mm_j_particles);
    }
    else /* ONIOM */ /* ????? */
    {
        mm->nrMMatoms = 0;
        /* do for each layer */
        for (j = 0; j < qr->nrQMlayers; j++)
        {
            qm             = qr->qm[j];
            qm->shiftQM[0] = XYZ2IS(0, 0, 0);
            for (i = 1; i < qm->nrQMatoms; i++)
            {
                qm->shiftQM[i] = pbc_dx_aiuc(&pbc, x[qm->indexQM[0]], x[qm->indexQM[i]], dx);
            }
            update_QMMM_coord(x, fr, qm, mm);
        }
    }
} /* update_QMMM_rec */

real calculate_QMMM(const t_commrec* cr, gmx::ForceWithShiftForces* forceWithShiftForces, const t_QMMMrec* qr)
{
    real QMener = 0.0;
    /* a selection for the QM package depending on which is requested
     * (Gaussian, GAMESS-UK, MOPAC or ORCA) needs to be implemented here. Now
     * it works through defines.... Not so nice yet
     */
    t_QMrec *qm, *qm2;
    t_MMrec* mm     = nullptr;
    rvec *   forces = nullptr, *fshift = nullptr, *forces2 = nullptr,
         *fshift2 = nullptr; /* needed for multilayer ONIOM */
    int i, j, k;

    if (!GMX_QMMM)
    {
        gmx_incons("Compiled without QMMM");
    }

    /* make a local copy the QMMMrec pointer
     */
    mm = qr->mm;

    /* now different procedures are carried out for one layer ONION and
     * normal QMMM on one hand and multilayer oniom on the other
     */
    gmx::ArrayRef<gmx::RVec> fMM      = forceWithShiftForces->force();
    gmx::ArrayRef<gmx::RVec> fshiftMM = forceWithShiftForces->shiftForces();
    if (qr->QMMMscheme == eQMMMschemenormal || qr->nrQMlayers == 1)
    {
        qm = qr->qm[0];
        snew(forces, (qm->nrQMatoms + mm->nrMMatoms));
        snew(fshift, (qm->nrQMatoms + mm->nrMMatoms));
        QMener = call_QMroutine(cr, qr, qm, mm, forces, fshift);
        for (i = 0; i < qm->nrQMatoms; i++)
        {
            for (j = 0; j < DIM; j++)
            {
                fMM[qm->indexQM[i]][j] -= forces[i][j];
                fshiftMM[qm->shiftQM[i]][j] += fshift[i][j];
            }
        }
        for (i = 0; i < mm->nrMMatoms; i++)
        {
            for (j = 0; j < DIM; j++)
            {
                fMM[mm->indexMM[i]][j] -= forces[qm->nrQMatoms + i][j];
                fshiftMM[mm->shiftMM[i]][j] += fshift[qm->nrQMatoms + i][j];
            }
        }
        free(forces);
        free(fshift);
    }
    else /* Multi-layer ONIOM */
    {
        for (i = 0; i < qr->nrQMlayers - 1; i++) /* last layer is special */
        {
            qm  = qr->qm[i];
            qm2 = copy_QMrec(qr->qm[i + 1]);

            qm2->nrQMatoms = qm->nrQMatoms;

            for (j = 0; j < qm2->nrQMatoms; j++)
            {
                for (k = 0; k < DIM; k++)
                {
                    qm2->xQM[j][k] = qm->xQM[j][k];
                }
                qm2->indexQM[j]        = qm->indexQM[j];
                qm2->atomicnumberQM[j] = qm->atomicnumberQM[j];
                qm2->shiftQM[j]        = qm->shiftQM[j];
            }

            qm2->QMcharge = qm->QMcharge;
            /* this layer at the higher level of theory */
            srenew(forces, qm->nrQMatoms);
            srenew(fshift, qm->nrQMatoms);
            /* we need to re-initialize the QMroutine every step... */
            init_QMroutine(cr, qm, mm);
            QMener += call_QMroutine(cr, qr, qm, mm, forces, fshift);

            /* this layer at the lower level of theory */
            srenew(forces2, qm->nrQMatoms);
            srenew(fshift2, qm->nrQMatoms);
            init_QMroutine(cr, qm2, mm);
            QMener -= call_QMroutine(cr, qr, qm2, mm, forces2, fshift2);
            /* E = E1high-E1low The next layer includes the current layer at
             * the lower level of theory, which provides + E2low
             * this is similar for gradients
             */
            for (i = 0; i < qm->nrQMatoms; i++)
            {
                for (j = 0; j < DIM; j++)
                {
                    fMM[qm->indexQM[i]][j] -= (forces[i][j] - forces2[i][j]);
                    fshiftMM[qm->shiftQM[i]][j] += (fshift[i][j] - fshift2[i][j]);
                }
            }
            free(qm2);
        }
        /* now the last layer still needs to be done: */
        qm = qr->qm[qr->nrQMlayers - 1]; /* C counts from 0 */
        init_QMroutine(cr, qm, mm);
        srenew(forces, qm->nrQMatoms);
        srenew(fshift, qm->nrQMatoms);
        QMener += call_QMroutine(cr, qr, qm, mm, forces, fshift);
        for (i = 0; i < qm->nrQMatoms; i++)
        {
            for (j = 0; j < DIM; j++)
            {
                fMM[qm->indexQM[i]][j] -= forces[i][j];
                fshiftMM[qm->shiftQM[i]][j] += fshift[i][j];
            }
        }
        free(forces);
        free(fshift);
        free(forces2);
        free(fshift2);
    }
    return (QMener);
} /* calculate_QMMM */

#pragma GCC diagnostic pop
