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

    int nafc = 0;               // number of times after file close callback was called

    // wait for data to be ready
    if (passthru && !metadata && !shared)
    {
        for (auto& intercomm: intercomms)
            diy_comm(intercomm).barrier();
    }
    fmt::print(stderr, "*** consumer after barrier completed! ***\n");

    if (shared)                     // single process, MetadataVOL test
        fmt::print(stderr, "consumer: using shared mode MetadataVOL plugin created by prod-con\n");
    else                            // normal multiprocess, DistMetadataVOL plugin
    {
        l5::DistMetadataVOL& vol_plugin = l5::DistMetadataVOL::create_DistMetadataVOL(local, intercomms);

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

        vol_plugin.set_keep(true);
        vol_plugin.serve_on_close = false;

        // even though this is the consumer, it produces a file for debugging at the end of its execution
        // hence, the producer-like callbacks below when the filename is outfile
        // however, no after_file_close callback because the outfile is always passthru (file on disk)

        // set a callback to broadcast/receive files before a file open
        vol_plugin.set_before_file_open([&](const std::string& name)
        {
            if (name == outfile)
                vol_plugin.broadcast_files();
        });
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
    rval = mbi->write_file(outfile.c_str(), 0, write_opts.c_str(), &root, 1); ERR(rval);
    fmt::print(stderr, "*** consumer wrote the file for debug ***\n");
}

