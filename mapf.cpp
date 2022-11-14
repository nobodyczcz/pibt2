#include <getopt.h>

#include <default_params.hpp>
#include <hca.hpp>
#include <iostream>
#include <pibt.hpp>
#include <pibt_plus.hpp>
#include <problem.hpp>
#include <push_and_swap.hpp>
#include <random>
#include <vector>

void printHelp();
std::unique_ptr<MAPF_Solver> getSolver(const std::string solver_name,
                                       MAPF_Instance* P, bool verbose, int argc,
                                       char* argv[]);

int main(int argc, char* argv[])
{
  std::string map_file = "";
  std::string agent_file = "";
  int agentsNum = 0;

  std::string output_file = DEFAULT_OUTPUT_FILE;
  std::string path_file = DEFAULT_OUTPUT_FILE;

  std::string solver_name;
  bool verbose = false;
  char* argv_copy[argc + 1];
  for (int i = 0; i < argc; ++i) argv_copy[i] = argv[i];

  struct option longopts[] = {
      {"map",required_argument,0,'m'},
      {"agents", required_argument,0,'a'},
      {"agentNum",required_argument,0,'k'},
      {"output", required_argument, 0, 'o'},
      {"outputPaths", optional_argument, 0, 'p'},

      {"solver", required_argument, 0, 's'},
      {"seed", optional_argument, 0, 'd'},
      {"verbose", no_argument, 0, 'v'},
      {"help", no_argument, 0, 'h'},
      {"time-limit", required_argument, 0, 't'},
      {"timestep-limit", optional_argument, 0, 'e'},

      {"log-short", no_argument, 0, 'L'},
      {"make-scen", no_argument, 0, 'P'},
      {0, 0, 0, 0},
  };
  bool make_scen = false;
  bool log_short = false;
  int max_comp_time = -1;
  int max_timestep = -1;
  // command line args
  int opt, longindex, random_seed;
  opterr = 0;  // ignore getopt error
  while ((opt = getopt_long(argc, argv, "m:a:k:o:p:s:d:t:e:vhPL", longopts,
                            &longindex)) != -1) {
    switch (opt) {
      case 'm':
        map_file = std::string(optarg);
        break;
      case 'a':
        agent_file = std::string(optarg);
        break;
      case 'k':
        agentsNum = std::atoi(optarg);
        break;
      case 'o':
        output_file = std::string(optarg)+".csv";
        break;
      case 'p':
        path_file =  std::string(optarg)+".path";
        break;
      case 's':
        solver_name = std::string(optarg);
        break;
      case 'd':
        random_seed = std::atoi(optarg);
        break;
      case 't':
        max_comp_time = std::atoi(optarg) * 1000;
        break;
      case 'e':
        max_timestep = std::atoi(optarg);
        break;
      case 'v':
        verbose = true;
        break;
      case 'h':
        printHelp();
        return 0;
      case 'P':
        make_scen = true;
        break;
      case 'L':
        log_short = true;
        break;
      default:
        break;
    }
  }

  if (map_file.length() == 0 ||  agent_file.length()==0) {
    std::cout << "specify map file using -m [MAPF-FILE], agent file using -a [AGENT-FILE]"
              << std::endl;
    return 0;
  }

  // set problem
  auto P = MAPF_Instance(map_file,agent_file,agentsNum, max_comp_time,max_timestep,random_seed);

  // set max computation time (otherwise, use param in instance_file)
  if (max_comp_time != -1) P.setMaxCompTime(max_comp_time);

  // create scenario
  if (make_scen) {
    P.makeScenFile(output_file);
    return 0;
  }

  // solve
  auto solver = getSolver(solver_name, &P, verbose, argc, argv_copy);
  solver->setLogShort(log_short);
  solver->solve();
  if (solver->succeed() && !solver->getSolution().validate(&P)) {
    std::cout << "error@mapf: invalid results" << std::endl;
    return 0;
  }
  solver->printResult();

  // output result
  solver->makeLog(output_file, path_file);
  if (verbose) {
    std::cout << "save result as " << output_file << std::endl;
  }

  return 0;
}

std::unique_ptr<MAPF_Solver> getSolver(const std::string solver_name,
                                       MAPF_Instance* P, bool verbose, int argc,
                                       char* argv[])
{
  std::unique_ptr<MAPF_Solver> solver;
  if (solver_name == "PIBT") {
    solver = std::make_unique<PIBT>(P);
  } else if (solver_name == "HCA") {
    solver = std::make_unique<HCA>(P);
  } else if (solver_name == "PIBT_PLUS") {
    solver = std::make_unique<PIBT_PLUS>(P);
  } else if (solver_name == "PushAndSwap") {
    solver = std::make_unique<PushAndSwap>(P);
  } else {
    std::cout << "warn@mapf: "
              << "unknown solver name, " + solver_name + ", continue by PIBT"
              << std::endl;
    solver = std::make_unique<PIBT>(P);
  }
  solver->setParams(argc, argv);
  solver->setVerbose(verbose);
  return solver;
}

void printHelp()
{
  std::cout << "\nUsage: ./mapf [OPTIONS] [SOLVER-OPTIONS]\n"
            << "\n**instance file is necessary to run MAPF simulator**\n\n"
            << "  -m --map [FILE_PATH]     map file path\n"
            << "  -a --agent [FILE_PATH]     scenario file path\n"
            << "  -e --timestep-limit [FILE_PATH]     max timestep\n"
            << "  -o --output [FILE_PATH]       output file path \n"
            << "  -p --outputPaths [FILE_PATH]       paths to output file \n"

            << "  -v --verbose                  print additional info\n"
            << "  -h --help                     help\n"
            << "  -s --solver [SOLVER_NAME]     solver, choose from the below\n"
            << "  -t --time-limit [INT]         max computation time (s)\n"
            << "  -L --log-short                use short log"
            << "  -P --make-scen                make scenario file using "
               "random starts/goals"
            << "\n\nSolver Options:" << std::endl;
  // each solver
  PIBT::printHelp();
  HCA::printHelp();
  PIBT_PLUS::printHelp();
  PushAndSwap::printHelp();
}
