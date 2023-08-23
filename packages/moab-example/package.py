# Copyright 2013-2020 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack import *


class MoabExample(CMakePackage):
    """Example of Testing Moab with LowFive."""

    homepage = "https://github.com/tpeterka/moab-example.git"
    url      = "https://github.com/tpeterka/moab-example.git"
    git      = "https://github.com/tpeterka/moab-example.git"

#     version('main', branch='main')
    version('hdf5-1.14', branch='hdf5-1.14')

    depends_on('mpich')
    depends_on('hdf5+mpi+hl', type='link')
    depends_on('moab', type='link')
#     depends_on('lowfive', type='link')

    def cmake_args(self):
        args = ['-DCMAKE_C_COMPILER=%s' % self.spec['mpich'].mpicc,
                '-DCMAKE_CXX_COMPILER=%s' % self.spec['mpich'].mpicxx,
                '-DBUILD_SHARED_LIBS=false']
#                 '-DLOWFIVE_PATH=%s' % self.spec['lowfive'].prefix]
        return args
