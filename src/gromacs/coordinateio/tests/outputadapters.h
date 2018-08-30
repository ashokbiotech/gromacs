/*
 * This file is part of the GROMACS molecular simulation package.
 *
 * Copyright (c) 2019, by the GROMACS development team, led by
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
/*!\file
 * \libinternal
 * \brief
 * Helpers and data for outputadapter module tests.
 *
 * \author Paul Bauer <paul.bauer.q@gmail.com>
 * \inlibraryapi
 * \ingroup module_coordinateio
 */

#ifndef GMX_COORDINATEIO_TESTS_MODULE_H
#define GMX_COORDINATEIO_TESTS_MODULE_H

#include "gmxpre.h"

#include "config.h"

#include <gtest/gtest.h>

#include "gromacs/coordinateio/outputadapters/outputselector.h"
#include "gromacs/coordinateio/outputadapters/setatoms.h"
#include "gromacs/math/vec.h"

#include "gromacs/coordinateio/tests/coordinate_test.h"

namespace gmx
{

namespace test
{

/*!\libinternal \brief  Helper to test supported file names. */
class SetAtomsSupportedFiles : public ModuleTest
{
    public:
        void prepareTest(const char *filename)
        {
            addTopology();
            //! Storage for requirements.
            OutputRequirements requirements;

            requirements.atoms = ChangeAtomsType::AlwaysFromStructure;

            EXPECT_NO_THROW(runTest(filename, requirements));
        }
};

/*!\libinternal \brief  Helper to test supported file names. */
class SetAtomsUnSupportedFiles : public ModuleTest
{
    public:
        void prepareTest(const char *filename)
        {
            //! Storage for requirements.
            OutputRequirements requirements;

            requirements.atoms = ChangeAtomsType::AlwaysFromStructure;

            EXPECT_THROW(runTest(filename, requirements), InconsistentInputError);
        }
};

/*!\libinternal \brief  Helper to test supported file names. */
class AnyOutputSupportedFiles : public ModuleTest,
                                public ModuleSelection
{
    public:
        void prepareTest(const char *filename)
        {
            addTopology();
            //! Storage for requirements.
            OutputRequirements requirements;
            //! Local atoms
            Selection          sel;
            //! Local box
            matrix             box;

            clear_mat(box);

            addOptionForSelection(&dummySelection_, true);
            setSelectionOptionValues(getOption(), &dummySelection_, true);

            copy_mat(requirements.newBox, box);
            requirements.box       = ChangeFrameInfoType::Always;
            requirements.frameTime = ChangeFrameTimeType::Both;

            EXPECT_NO_THROW(runTest(filename, requirements));
        }
};

/*!\libinternal \brief  Helper to test that invalid selection is rejected */
class OutputSelectorDeathTest : public ModuleTest,
                                public ModuleSelection
{
    public:
        void prepareTest()
        {
            //! Storage for frameadapters.
            OutputAdapterContainer adapters(CoordinateFileFlags::Base);
            //! Local atoms
            Selection              sel;

            addOptionForSelection(&sel, false);
            setSelectionOptionValues(getOption(), &sel, false);

            ASSERT_DEATH_IF_SUPPORTED(
                    adapters.addAdapter(std::make_unique<OutputSelector>(sel),
                                        CoordinateFileFlags::RequireCoordinateSelection),
                    "Need a valid selection out of simple atom indices");
        }
};

//! Names here work for setAtoms module
const char *const setAtomsSupported[] = {
#if GMX_USE_TNG
    "spc2-traj.tng",
#endif
    "spc2-traj.gro",
    "spc2-traj.pdb",
};

//! Names here don't work for setAtoms module
const char *const setAtomsUnSupported[] = {
    "spc2-traj.trr",
    "spc2-traj.xtc",
    "spc2-traj.g96"
};

/*! \brief
 *  Names here work for stuff that has no specific requirements.
 *
 *  PDB and GRO format are not tested here because they also require atoms information
 *  that is incompatible with the other output formats.
 */
const char *const anySupported[] = {
    "spc2-traj.trr",
#if GMX_USE_TNG
    "spc2-traj.tng",
#endif
    "spc2-traj.xtc",
    "spc2-traj.g96"
};

} // namespace test

} // namespace gmx

#endif