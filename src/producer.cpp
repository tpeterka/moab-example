#include "prod-con.hpp"

// prints mesh statistics (for debugging)
void PrintMeshStats(Interface *mbint,        // moab interface
        EntityHandle *mesh_set,  // moab mesh set
        ParallelComm *mbpc)      // moab parallel communicator
{
    Range range;
    ErrorCode rval;
    static int mesh_num = 0;                 // counts how many time this function is called
    float loc_verts = 0.0;                   // num local verts (fractional for shared verts)
    float glo_verts;                         // global number of verts
    float loc_share_verts = 0.0;             // num local shared verts (fractional)
    float glo_share_verts;                   // global number of shared verts
    int loc_cells, glo_cells;                // local and global number of cells (no sharing)
    int rank;

    MPI_Comm_rank(mbpc->comm(), &rank);

    // get local quantities
    range.clear();
    rval = mbint->get_entities_by_dimension(*mesh_set, 0, range); ERR;

    // compute fractional contribution of shared vertices attributed to this proc
    int ps[MAX_SHARING_PROCS];               // sharing procs for a vert
    EntityHandle hs[MAX_SHARING_PROCS];      // handles of shared vert on sharing procs
    int num_ps = 0;                          // number of sharing procs, returned by moab
    unsigned char pstat;                     // pstatus, returned by moab
    for (Range::iterator verts_it = range.begin(); verts_it != range.end();
            verts_it++)
    {
        rval = mbpc->get_sharing_data(*verts_it, ps, hs, pstat, num_ps); ERR;
        if (num_ps == 0)
            loc_verts++;
        else if (num_ps == 1)                // when 2 procs, moab lists only one (the other)
        {
            loc_verts += 0.5;
            loc_share_verts += 0.5;
        }
        else
        {
            loc_verts += (1.0 / (float)num_ps);
            loc_share_verts += (1.0 / (float)num_ps);
        }
    }

    range.clear();
    rval = mbint->get_entities_by_dimension(*mesh_set, 3, range); ERR;
    loc_cells = (int)range.size();

    // add totals for global quantities
    MPI_Reduce(&loc_verts, &glo_verts, 1, MPI_FLOAT, MPI_SUM, 0,
            mbpc->comm());
    MPI_Reduce(&loc_share_verts, &glo_share_verts, 1, MPI_FLOAT, MPI_SUM, 0,
            mbpc->comm());
    MPI_Reduce(&loc_cells, &glo_cells, 1, MPI_INT, MPI_SUM, 0, mbpc->comm());

    // report results
    if (rank == 0)
    {
        fmt::print(stderr, "----------------- Mesh {} statistics -----------------\n", mesh_num);
        fmt::print(stderr, "Total number of verts = {} of which {} "
                "are shared\n", glo_verts, glo_share_verts);
        fmt::print(stderr, "Total number of cells = {}\n", glo_cells);
        fmt::print(stderr, "------------------------------------------------------\n");
    }

    mesh_num = (mesh_num + 1) % 2;
}

// prepares the mesh by decomposing source domain and creating mesh in situ
// and creating meshes in situ
void PrepMesh  (int src_type,
                int src_size,
                int slab,
                Interface* mbi,
                ParallelComm* pc,
                EntityHandle root,
                bool debug = false)
{
    diy::RegularDecomposer<Bounds>::CoordinateVector    ghost(3, 0);
    diy::RegularDecomposer<Bounds>::DivisionsVector     given(3, 0);
    std::vector<bool>                                   share_face(3, true);
    std::vector<bool>                                   wrap(3, false);

    diy::mpi::communicator comm = pc->comm();

    int src_mesh_size[3]  = {src_size, src_size, src_size};      // source size
    Bounds domain(3);
    domain.min[0] = domain.min[1] = domain.min[2] = 0;
    domain.max[0] = domain.max[1] = domain.max[2] = src_size - 1;

    // decompose domain
    if (slab == 1)
    {
        // blocks are slabs in the z direction
        given[1] = 1;
    }

    // the following calls to asssigners are for 1 block per process
    diy::RoundRobinAssigner         assigner(comm.size(), comm.size());
    diy::RegularDecomposer<Bounds>  decomposer(3,
                                               domain,
                                               assigner.nblocks(),
                                               share_face,
                                               wrap,
                                               ghost,
                                               given);

    // report the number of blocks in each dimension of each mesh
    if (comm.rank() == 0)
        fmt::print(stderr, "Number of blocks in source = [{}]\n", fmt::join(decomposer.divisions, ","));

    // create mesh in situ
    if (debug)
        pc->set_debug_verbosity(5);

    if (src_type == 0)
        hex_mesh_gen(src_mesh_size, mbi, &root, pc, decomposer, assigner);
    else
        tet_mesh_gen(src_mesh_size, mbi, &root, pc, decomposer, assigner);


    // debug: print mesh stats
    PrintMeshStats(mbi, &root, pc);

//     // add field to input mesh
//     PutVertexField(mbi, roots[0], "vertex_field", factor);
//     PutElementField(mbi, roots[0], "element_field", factor);
}

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

    // debug
    fmt::print(stderr, "producer: local comm rank {} size {}\n", local_.rank(), local_.size());

    // VOL plugin and properties
    hid_t plist;

    if (shared)                 // single process, MetadataVOL test
        fmt::print(stderr, "producer: using shared mode MetadataVOL plugin created by prod-con\n");
    else                        // normal multiprocess, DistMetadataVOL plugin
    {
        l5::DistMetadataVOL& vol_plugin = l5::DistMetadataVOL::create_DistMetadataVOL(local, intercomms);
        plist = H5Pcreate(H5P_FILE_ACCESS);

        if (passthru)
            H5Pset_fapl_mpio(plist, local, MPI_INFO_NULL);

        l5::H5VOLProperty vol_prop(vol_plugin);
        if (!getenv("HDF5_VOL_CONNECTOR"))
            vol_prop.apply(plist);

        // set lowfive properties
        if (passthru)
            vol_plugin.set_passthru("example1.h5", "*");
        if (metadata)
            vol_plugin.set_memory("example1.h5", "*");
    }


    // debug
    fmt::print(stderr, "*** producer generating moab mesh ***\n");

    // create moab mesh
    int                             mesh_type = 0;                          // source mesh type (0 = hex, 1 = tet)
    int                             mesh_size = 100;                        // source mesh size per side
    int                             mesh_slab = 0;                          // block shape (0 = cubes; 1 = slabs)
    Interface*                      mbi = new Core();                       // moab interface
    ParallelComm*                   pc  = new ParallelComm(mbi, local);     // moab communicator TODO: necessary?
    EntityHandle                    root;
    mbi->create_meshset(MESHSET_SET, root);
    PrepMesh(mesh_type, mesh_size, mesh_slab, mbi, pc, root);

    // TODO: write file

    // debug
    fmt::print(stderr, "*** producer after closing file ***\n");

    if (!shared)
        H5Pclose(plist);

    // signal the consumer that data are ready
    if (passthru && !metadata && !shared)
    {
        for (auto& intercomm: intercomms)
            diy_comm(intercomm).barrier();
    }
}
