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
    std::string infile      = "example1_2parts.h5m";
    std::string read_opts   = "PARALLEL=READ_PART;PARTITION=PARALLEL_PARTITION;PARALLEL_RESOLVE_SHARED_ENTS";

    // debug
    fmt::print(stderr, "consumer: local comm rank {} size {} metadata {} passthru {}\n",
            local_.rank(), local_.size(), metadata, passthru);

    // wait for data to be ready
    if (passthru && !metadata && !shared)
    {
        for (auto& intercomm: intercomms)
            diy_comm(intercomm).barrier();
    }

    // VOL plugin and properties
    hid_t plist;

    if (shared)                     // single process, MetadataVOL test
        fmt::print(stderr, "consumer: using shared mode MetadataVOL plugin created by prod-con\n");
    else                            // normal multiprocess, DistMetadataVOL plugin
    {
//         l5::DistMetadataVOL& vol_plugin = l5::DistMetadataVOL::create_DistMetadataVOL(local, intercomms);
        plist = H5Pcreate(H5P_FILE_ACCESS);

        if (passthru)
            H5Pset_fapl_mpio(plist, local, MPI_INFO_NULL);

//         l5::H5VOLProperty vol_prop(vol_plugin);
//         if (!getenv("HDF5_VOL_CONNECTOR"))
//             vol_prop.apply(plist);
// 
//         // set lowfive properties
//         if (passthru)
//         {
//             // debug
//             fmt::print(stderr, "*** consumer setting passthru mode\n");
// 
//             vol_plugin.set_passthru(infile, "*");
//         }
//         if (metadata)
//         {
//             // debug
//             fmt::print(stderr, "*** consumer setting memory mode\n");
// 
//             vol_plugin.set_memory(infile, "*");
//         }
//         vol_plugin.set_intercomm(infile, "*", 0);
    }

    // initialize moab
    Interface*                      mbi = new Core();                       // moab interface
    EntityHandle                    root;
    ErrorCode                       rval;
    rval = mbi->create_meshset(MESHSET_SET, root); ERR(rval);

    // debug
    fmt::print(stderr, "*** consumer before opening file ***\n");

    // read file
    rval = mbi->load_file(infile.c_str(), &root, read_opts.c_str() ); ERR(rval);

    // clean up
    if (!shared)
        H5Pclose(plist);

    // debug
    fmt::print(stderr, "*** consumer after closing file ***\n");
}

