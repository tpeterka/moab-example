#include <diy/mpi/communicator.hpp>
#include <thread>
#include "prod-con.hpp"

extern "C"
{
void consumer_f (
        communicator& local,
        const std::vector<communicator>& intercomms,
        bool shared,
        int metadata,
        int passthru);
}

void consumer_f (
        communicator& local,
        const std::vector<communicator>& intercomms,
        bool shared,
        int metadata,
        int passthru)
{
    diy::mpi::communicator local_(local);
    std::string infile      = "example1.h5m";
    std::string read_opts   = "PARALLEL=READ_PART;PARTITION=PARALLEL_PARTITION;PARALLEL_RESOLVE_SHARED_ENTS;DEBUG_IO=3;";
    std::string outfile     = "example1_cons.h5m";      // for debugging
    std::string write_opts  = "PARALLEL=WRITE_PART;DEBUG_IO=6";

    // debug
    fmt::print(stderr, "consumer: local comm rank {} size {} metadata {} passthru {}\n",
            local_.rank(), local_.size(), metadata, passthru);

    // wait for data to be ready
    if (passthru && !metadata && !shared)
    {
        for (auto& intercomm: intercomms)
            diy_comm(intercomm).barrier();
    }
    fmt::print(stderr, "*** consumer after barrier completed! ***\n");

    // VOL plugin and properties
    hid_t plist;

    if (shared)                     // single process, MetadataVOL test
    {

#ifdef LOWFIVE_PATH

        fmt::print(stderr, "consumer: using shared mode MetadataVOL plugin created by prod-con\n");

#endif

    }
    else                            // normal multiprocess, DistMetadataVOL plugin
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
            fmt::print(stderr, "*** consumer setting passthru mode\n");

            vol_plugin.set_passthru(infile, "*");
        }
        if (metadata)
        {
            // debug
            fmt::print(stderr, "*** consumer setting memory mode\n");

            vol_plugin.set_memory(infile, "*");
        }
        vol_plugin.set_passthru(outfile, "*");      // outfile for debugging goes to disk
        vol_plugin.set_intercomm(infile, "*", 0);

        // set a callback to broadcast/receive files by other before a file open
//         static int nopen = 0;            // needs to be static to be captured correctly in lambda, not sure why
//         vol_plugin.set_before_file_open([&]()
//         {
//             fmt::print(stderr, "--- Consumer entering before file open callback nopen = {} ---\n", nopen);
// 
//             if (local_.rank() == 0 && nopen == 3)
//             {
//                 fmt::print(stderr, "--- consumer root, nopen = {}, broadcast files ---\n", nopen);
//                 vol_plugin.broadcast_files();
//             }
//             if (local_.rank() > 0 && nopen == 1)
//             {
//                 fmt::print(stderr, "--- consumer nonroot nopen = {} receive broadcasted files ---\n", nopen);
//                 vol_plugin.broadcast_files();
//             }
// 
//             nopen++;
// 
//             fmt::print(stderr, "--- Consumer leaving before file open callback nopen = {} ---\n", nopen);
//         });

        vol_plugin.set_keep(true);
#endif

    }

    // initialize moab
    Interface*                      mbi = new Core();                       // moab interface
    ParallelComm*                   pc  = new ParallelComm(mbi, local);     // moab communicator
    EntityHandle                    root;
    ErrorCode                       rval;
    rval = mbi->create_meshset(MESHSET_SET, root); ERR(rval);

    // debug
    fmt::print(stderr, "*** consumer before reading file ***\n");

    // read file
    rval = mbi->load_file(infile.c_str(), &root, read_opts.c_str() ); ERR(rval);

    // debug
    fmt::print(stderr, "*** consumer after reading file ***\n");

    // write file for debugging
//     rval = mbi->write_file(outfile.c_str(), 0, write_opts.c_str(), &root, 1); ERR(rval);
//     fmt::print(stderr, "*** consumer wrote the file for debug ***\n");

    // clean up
    if (!shared)
        H5Pclose(plist);
}

