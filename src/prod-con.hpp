#pragma once

#include    <vector>
#include    <cassert>
#include    <thread>
#include    <mutex>

// lowfive
#include    <lowfive/vol-metadata.hpp>
#include    <lowfive/vol-dist-metadata.hpp>
#include    <lowfive/log.hpp>
#include    <lowfive/H5VOLProperty.hpp>

// moab
#include "iMesh.h"
#include "MBiMesh.hpp"
#include "moab/Core.hpp"
#include "moab/Range.hpp"
#include "MBTagConventions.hpp"
#include "moab/ParallelComm.hpp"
#include "moab/HomXform.hpp"
#include "moab/ReadUtilIface.hpp"

using communicator  = MPI_Comm;
using diy_comm      = diy::mpi::communicator;

namespace l5        = LowFive;

enum {producer_task, producer1_task, producer2_task, consumer_task, consumer1_task, consumer2_task};

