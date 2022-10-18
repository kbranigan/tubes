
#include "block.h"
#include "../ext/osmpbfreader.h"
using namespace osmpbfreader;

struct Block * nodes = NULL;
struct Block * ways = NULL;
struct Block * nds = NULL;

struct Counter {
  int nNodes;
  int nWays;
  int nRelations;

  Counter() {} // : nodes(0), ways(0), relations(0) {}

  // This method is called every time a Node is read
  void node_callback(uint64_t osmid, double lon, double lat, const Tags & tags) {
    // nodes = add_row(nodes);
    // set_cell_from_int64(nodes,  nodes->num_rows-1, 0, osmid); // id
    // set_cell_from_double(nodes, nodes->num_rows-1, 1, lat);   // lat
    // set_cell_from_double(nodes, nodes->num_rows-1, 2, lon);   // lon
    // set_cell_from_double(nodes, nodes->num_rows-1, 3, 0.0);   // elevation
    // set_cell_from_int32(nodes,  nodes->num_rows-1, 4, 0);     // num_ways
    // fprintf(stderr, "%f %f\n", lon, lat);
    // exit(1);
    ++nNodes;
  }

  // This method is called every time a Way is read
  // refs is a vector that contains the reference to the nodes that compose the way
  void way_callback(uint64_t osmid, const Tags &tags, const std::vector<uint64_t> &refs) {
    ++nWays;

    // if (tags.find("highway") != tags.end()) {
    //   ways.push_back(refs);
    // }

    for (auto it = tags.begin(); it != tags.end() ; it++) {
      fprintf(stderr, "%s: %s\n", it->first.c_str(), it->second.c_str());
    }

    fprintf(stderr, "%ld: %ld (%ld)\n", osmid, refs.size(), tags.size());
    exit(1);
  }

  // This method is called every time a Relation is read
  // refs is a vector of pair corresponding of the relation type (Node, Way, Relation) and the reference to the object
  void relation_callback(uint64_t osmid, const Tags &tags, const References & refs) {
    ++nRelations;
  }
};

int main(int argc, char ** argv)
{
  static char filename[1000] = "";

  int c;
  while (1)
  {
    static struct option long_options[] = {
      {"filename", required_argument, 0, 'f'},
      {0, 0, 0, 0}
    };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "f:", long_options, &option_index);
    if (c == -1) break;
    
    switch (c)
    {
      case 0: break;
      case 'f': strncpy(filename, optarg, sizeof(filename)); break;
      default: abort();
    }
  }

  if (filename[0] == 0 && argc == 2 && argv[1] != NULL)
    strncpy(filename, argv[1], sizeof(filename));
  
  if (filename[0] == 0) { fprintf(stderr, "ERROR %s: filename not provided\n", argv[0]); return EXIT_FAILURE; }

  nodes = new_block();
  nodes = add_int64_column(nodes, "id");
  nodes = add_double_column(nodes, "lat");
  nodes = add_double_column(nodes, "lon");
  nodes = add_double_column(nodes, "elevation");
  nodes = add_int32_column(nodes, "num_ways");
  // block = add_int32_column(block, "shape_part_id");

  // int id_cid         = get_column_id_by_name_or_exit(nodes, "id");
  // int lat_cid        = get_column_id_by_name_or_exit(nodes, "lat");
  // int lon_cid        = get_column_id_by_name_or_exit(nodes, "lon");
  // int elevation_cid  = get_column_id_by_name(nodes, "elevation");
  // int num_ways_cid   = get_column_id_by_name(nodes, "num_ways");

  Counter counter;
  read_osm_pbf(argv[1], counter);
  std::cout << "We read " << counter.nNodes << " nodes, " << counter.nWays << " ways and " << counter.nRelations << " relations" << std::endl;

  // struct Block * block = bsv(filename, EXTRACT_DATA);
  //block = bsv(filename, EXTRACT_DATA, block);
  // write_block(stdout, block);
  FILE * nodes_fp = fopen("nodes.b", "wb");
  write_block(nodes_fp, nodes);
  free_block(nodes);
  fclose(nodes_fp);
  
  return EXIT_SUCCESS;
}
