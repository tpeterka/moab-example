#include "prod-con.hpp"

herr_t fail_on_hdf5_error(hid_t stack_id, void*)
{
    H5Eprint(stack_id, stderr);
    fmt::print(stderr, "An HDF5 error was detected. Terminating.\n");
    exit(1);
}

extern "C" {
void producer_f (
        communicator& local,
        const std::vector<communicator>& intercomms,
        bool shared,
        int metadata,
        int passthru);
}

void producer_f (
        communicator& local,
        const std::vector<communicator>& intercomms,
        bool shared,
        int metadata,
        int passthru)
{
    diy::mpi::communicator local_(local);
    std::string outfile     = "example1.h5m";
    std::string write_opts  = "PARALLEL=WRITE_PART;DEBUG_IO=6";

    // debug
    fmt::print(stderr, "producer: local comm rank {} size {} metadata {} passthru {}\n",
            local_.rank(), local_.size(), metadata, passthru);

    // VOL plugin and properties
    hid_t plist;

    if (shared)                 // single process, MetadataVOL test
    {

#ifdef LOWFIVE_PATH

        fmt::print(stderr, "producer: using shared mode MetadataVOL plugin created by prod-con\n");

#endif

    }
    else                        // normal multiprocess, DistMetadataVOL plugin
    {

#ifdef LOWFIVE_PATH

        l5::DistMetadataVOL& vol_plugin = l5::DistMetadataVOL::create_DistMetadataVOL(local, intercomms);

#endif

        plist = H5Pcreate(H5P_FILE_ACCESS);

        if (passthru)
            H5Pset_fapl_mpio(plist, local, MPI_INFO_NULL);

#ifdef LOWFIVE_PATH

        l5::H5VOLProperty vol_prop(vol_plugin);
        if (!getenv("HDF5_VOL_CONNECTOR"))
            vol_prop.apply(plist);

        // set lowfive properties
        if (passthru)
        {
            // debug
            fmt::print(stderr, "*** producer setting passthru mode\n");

            vol_plugin.set_passthru(outfile, "*");
        }
        if (metadata)
        {
            // debug
            fmt::print(stderr, "*** producer setting memory mode\n");

            vol_plugin.set_memory(outfile, "*");
        }
        vol_plugin.set_keep(true);

        // set a callback to broadcast/receive files by other before a file open
        static int nopen = 0;                   // needs to be static in order to be captured correctly by lambda, not sure why
        vol_plugin.set_before_file_open([&](const std::string& name)
        {
            if (nopen == 0)
                vol_plugin.broadcast_files();
            nopen++;
        });

#endif

    }


    // debug
    fmt::print(stderr, "*** producer generating moab mesh ***\n");

    // create moab mesh
    int                             mesh_type = 0;                          // source mesh type (0 = hex, 1 = tet)
    int                             mesh_size = 10;                        // source mesh size per side
    int                             mesh_slab = 0;                          // block shape (0 = cubes; 1 = slabs)
    double                          factor = 1.0;                           // scaling factor on field values
    Interface*                      mbi = new Core();                       // moab interface
    ParallelComm*                   pc  = new ParallelComm(mbi, local);     // moab communicator
    EntityHandle                    root;
    ErrorCode                       rval;
    rval = mbi->create_meshset(MESHSET_SET, root); ERR(rval);
    PrepMesh(mesh_type, mesh_size, mesh_slab, mbi, pc, root, factor, false);

    // write file
    rval = mbi->write_file(outfile.c_str(), 0, write_opts.c_str(), &root, 1); ERR(rval);

    // debug
    fmt::print(stderr, "*** producer after writing file ***\n");

    if (!shared)
        H5Pclose(plist);

    // signal the consumer that data are ready
    if (passthru && !metadata && !shared)
    {
        for (auto& intercomm: intercomms)
            diy_comm(intercomm).barrier();
    }
    fmt::print(stderr, "*** producer after barrier completed! ***\n");
}
