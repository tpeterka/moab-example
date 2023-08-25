# Instructions for Building and Running Moab Example

Installation is done through Spack. If you don't have Spack installed or if Spack is new to you, go [here](https://spack.readthedocs.io/en/latest/) first.

## Setting up Spack environment

### First time: create and load the Spack environment

```
git clone https://github.com/tpeterka/moab-example
cd /path/to/moab-example
source ./create-env.sh     # requires being in the same directory to work properly
```

### Subsequent times: load the Spack environment

```
source /path/to/moab-example/load-env.sh
```

-----

## Running the example

### Shared mode: one MPI rank, producer and consumer take turns on same rank

passthru mode
```
cd $MOAB_EXAMPLE_PATH/bin
mpiexec -n 1 ./prod-con -m 0 -f 1
```
memory mode
```
cd $MOAB_EXAMPLE_PATH/bin
mpiexec -n 1 ./prod-con
```
### Distributed mode: 4 MPI ranks, 2 rank producer + 2 rank consumer

passthru mode
```
cd $MOAB_EXAMPLE_PATH/bin
mpiexec -n 4 ./prod-con -m 0 -f 1
```
memory mode
```
cd $MOAB_EXAMPLE_PATH/bin
mpiexec -n 4 ./prod-con
```



