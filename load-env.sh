#!/bin/bash

# activate the environment
export SPACKENV=moab-example-env
spack env deactivate > /dev/null 2>&1
spack env activate $SPACKENV
echo "activated spack environment $SPACKENV"

echo "setting flags for building moab-example"
export LOWFIVE_PATH=`spack location -i lowfive`
export MOAB_EXAMPLE_PATH=`spack location -i moab-example`
export HENSON_PATH=`spack location -i henson`

echo "setting flags for running moab-example"
export LD_LIBRARY_PATH=$LOWFIVE_PATH/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$HENSON_PATH/lib:$LD_LIBRARY_PATH

export HDF5_PLUGIN_PATH=$LOWFIVE_PATH/lib
export HDF5_VOL_CONNECTOR="lowfive under_vol=0;under_info={};"


