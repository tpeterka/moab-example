#!/bin/bash

export SPACKENV=moab-example-env
export YAML=$PWD/env.yaml

# add custom spack repos
echo "adding custom spack repo for moab-example"
spack repo add . > /dev/null 2>&1
echo "adding spack repo for lowfive"
spack repo add lowfive > /dev/null 2>&1

# create spack environment
echo "creating spack environment $SPACKENV"
spack env deactivate > /dev/null 2>&1
spack env remove -y $SPACKENV > /dev/null 2>&1
spack env create $SPACKENV $YAML

# activate environment
echo "activating spack environment"
spack env activate $SPACKENV

# add moab-example in develop mode
spack develop moab-example@master
spack add moab-example

# install everything in environment
echo "installing dependencies in environment"
spack install

# reset the environment (workaround for spack behavior)
spack env deactivate
spack env activate $SPACKENV

# set build flags
echo "setting flags for building moab-example"
export LOWFIVE_PATH=`spack location -i lowfive`
export MOAB_EXAMPLE_PATH=`spack location -i moab-example`
export HENSON_PATH=`spack location -i henson`

# set LD_LIBRARY_PATH
echo "setting flags for running moab-example"
export LD_LIBRARY_PATH=$LOWFIVE_PATH/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$HENSON_PATH/lib:$LD_LIBRARY_PATH

export HDF5_PLUGIN_PATH=$LOWFIVE_PATH/lib
export HDF5_VOL_CONNECTOR="lowfive under_vol=0;under_info={};"

