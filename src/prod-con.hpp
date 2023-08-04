#pragma once

#include    <vector>
#include    <cassert>
#include    <thread>
#include    <mutex>

// diy
#include    <diy/mpi/communicator.hpp>
#include    <diy/decomposition.hpp>
#include    <diy/assigner.hpp>

// lowfive
#include    <lowfive/vol-metadata.hpp>
#include    <lowfive/vol-dist-metadata.hpp>
#include    <lowfive/log.hpp>
#include    <lowfive/H5VOLProperty.hpp>

// moab
#include    "iMesh.h"
#include    "MBiMesh.hpp"
#include    "moab/Core.hpp"
#include    "moab/Range.hpp"
#include    "MBTagConventions.hpp"
#include    "moab/ParallelComm.hpp"
#include    "moab/HomXform.hpp"
#include    "moab/ReadUtilIface.hpp"

using communicator  = MPI_Comm;
using diy_comm      = diy::mpi::communicator;
using Bounds        = diy::DiscreteBounds;

namespace l5        = LowFive;

enum {producer_task, producer1_task, producer2_task, consumer_task, consumer1_task, consumer2_task};

#define ERR {if(rval!=MB_SUCCESS)printf("MOAB error at line %d in %s\n", __LINE__, __FILE__);}

using namespace moab;

void hex_mesh_gen(int *mesh_size, Interface *mbint, EntityHandle *mesh_set,
        ParallelComm *mbpc, diy::RegularDecomposer<Bounds>& decomp, diy::Assigner& assign);
void tet_mesh_gen(int *mesh_size, Interface *mbint, EntityHandle *mesh_set,
        ParallelComm *mbpc, diy::RegularDecomposer<Bounds>& decomp, diy::Assigner& assign);
void create_hexes_and_verts(int *mesh_size, Interface *mbint, EntityHandle *mesh_set,
        diy::RegularDecomposer<Bounds>& decomp, diy::Assigner& assign, ParallelComm* mbpc);
void create_tets_and_verts(int *mesh_size, Interface *mbint, EntityHandle *mesh_set,
        diy::RegularDecomposer<Bounds>& decomp, diy::Assigner& assign, ParallelComm* mbpc);
void resolve_and_exchange(Interface *mbint, EntityHandle *mesh_set, ParallelComm *mbpc);


