#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <map>
#include <netdesign_factory.h>
#include <topo_factory.h>
#include <flow_factory.h>
#include <flow_gen.h>
#include <fstream>
#include <design_space_exp.h>
#include <eval_parameters.h>
#include <adaptive_routing.h>
#include <util.h>
#include <debug_config.h>
#include <aperiodic_server.h>
#include <bits/stdc++.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <pugixml.hpp>
#include <pugiconfig.hpp>

#include <dirent.h>   // For directory reading (POSIX)
#include <sys/types.h> // For DIR and struct dirent

#include <fusion.h>
#include <getopt.h>
using namespace mosek::fusion;
using namespace monty;
/*
------------------------------------
FLOW PLANNING 
------------------------------------
- Read in network specifications,
- Read in flow specifications: maintain 2 separate flow specifications:
a) with only Real-Time flows,
b) with both Real-Time flows and Aperiodic flows
- Calculate the delays of Real-Time flows on the network,
a) with delay-composition theorem
b) with fixed priority worst-case response time analysis
- Get Period and Server Parameters (ACTIVE MODE)
- Calculate the delays of Real-Time + Aperiodic flows on the network,
- Plot the increase in delays incurred by the Real-Time flows
------------------------------------
SCHEDULABILITY/UTILIZATION ANALYSIS
------------------------------------
- Check fraction of flows accepted for a given topology. Compare
schedulability for RealFlow vs. current Flow Planning Scheme
- Can the topology be tweaked to fix the end-to-end deadline for a given
set of number of switches and links
*/

void print_usage(const char* app_name) {
    std::cout << "Usage: " << app_name << " [OPTIONS]\n\n"
              << "Options:\n"
              << " -f, --file <path> Process a single XML topology file with specified number of flows\n"
              << "      Requires: -n <num_flows>\n"
              << " -d, --directory <path> Process all XML files in directory (design space exploration)\n"
              << "      Requires: -n <num_flows>\n"
              << " -a, --fullspec <path> Process a single XML topology file with in-built info on flow specs\n"
              << " -n, --numflows <number> Number of flows to generate\n"
              << " -h, --help Display this help message\n\n"
              << "Examples:\n"
              << " " << app_name << " -f topology_file.xml -n 10\n"
              << " " << app_name << " -d full_path_topologies -n 20\n"
              << " " << app_name << " -a topology_flow_file.xml\n"
              << std::endl;
}

usefulEdgesCountMap usefulEdgesCtMap;

int main(int argc, char* argv[]){
    uint num_flows = 0;
    int c;
    latency_scheme ds = latency_scheme::DCT;

    std::string single_topo_xml_path = "";
    std::string multi_topo_directory_path = "";
    std::string full_info_xml_path = "";
    bool single_topology_mode = false;
    bool multi_topology_mode = false;
    bool full_info_mode = false;

    int option_index;

    static struct option long_options[] = {
        {"--file", required_argument, 0, 'f'},
        {"--directory", required_argument, 0, 'd'},
        {"--fullspec", required_argument, 0, 'a'},
        {"--numflows", required_argument, 0, 'n'},
        {"--help", optional_argument, 0, 'h'},
        {0,0,0,0}
    };
    
    while((c = getopt_long(argc, argv, "f:d:a:n:h", long_options, &option_index)) != -1){
        // cool trick! if getopt options take an argument
    
        switch (c) {

            case 'f':
                    single_topology_mode = true;
                    single_topo_xml_path = optarg;
                    break;

            case 'd':
                    // std::cout<<"option d with "<<long_options[option_index].name;
                    // if (optarg)
                    //     std::cout<<" = "<<optarg;
                    // std::cout<<std::endl;
                    multi_topology_mode = true;
                    multi_topo_directory_path = optarg;
                    std::cout<<multi_topo_directory_path.c_str();
                    break;
            
            case 'a':
                    // std::cout<<"option a with "<<long_options[option_index].name;
                    // if (optarg)
                    //     std::cout<<" = "<<optarg;
                    // std::cout<<std::endl;
                    full_info_mode = true;
                    full_info_xml_path = optarg;
                    break;


            case 'n':
                    num_flows = atoi(optarg);
                    // std::cout<<"Number of flows = "<<num_flows<<std::endl;
                    break;
            
            case 'h':
                    print_usage(argv[0]);
                    return 0;

            default:
                    std::cout<<"?? getopt returned character code 0%o ??\n"<<c;
                    return -1;
        }

    }

    // Getting a timetsamp in string form to get unique filename
    auto time_t_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream timestamp_ss;
    timestamp_ss << std::put_time(std::localtime(&time_t_now), "%Y%m%d_%H%M%S");
    std::string timestamp = timestamp_ss.str();
    std::cout<< "Invoking "<<argv[0]<<" at "<<timestamp<<" with "<<argc<<" arguments "<<"\n";

    // Error handling
    if ((single_topology_mode && multi_topology_mode) || (multi_topology_mode && full_info_mode) || (full_info_mode && single_topology_mode)){
        std::cerr << "Error: Cannot use multiple optpions among -f, -d, -a simultaneously\n" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    if (!single_topology_mode && !multi_topology_mode && !full_info_mode) {
        std::cerr << "Error: Must specify either -f or -d or -a option\n" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    if ((num_flows == 0) && (!full_info_mode)) {
        std::cerr << "Error: Must specify number of flows using -n option unless its a full specification mode\n" << std::endl;
        print_usage(argv[0]);
        return 1;
    }
    
    std::string output_dirname_fullpath = RES_DIR + timestamp + "/";
    const char* output_dirname_fullpath_cstr = output_dirname_fullpath.c_str();
    mkdir(output_dirname_fullpath_cstr, 0755); // no error checking for now To Do later


    // For all modes
    std::pair<double, double> acceptance_ratio;
    std::map<std::pair<double, int>, std::vector<std::pair<double, double>>> acceptance_ratios_map;

    // For Multi-Topology Mode
    DIR* dir;
    struct dirent* entry;
    struct dirent** e;
    int num_entries;
    int count = 0;

    std::map<double, std::vector<double>> dc_acceptance_ratios;
    std::map<double, std::vector<double>> rta_acceptance_ratios;


    // Mode 1: Single Topology
    if (single_topology_mode) {
        std::cout << "Processing single topology XML file: " << single_topo_xml_path << std::endl;
        std::cout << "Number of flows: " << num_flows << std::endl;
        
        // For a single flow specification with flow deadline choosen within [deadline_min, deadline_max]
        // acceptance_ratio = design_given_topo_specs(single_topo_xml_path.c_str(), num_flows, output_dirname_fullpath);
        // std::cout << "Acceptance Ratio (DC): " << acceptance_ratio.first << std::endl;
        // std::cout << "Acceptance Ratio (RTA): " << acceptance_ratio.second << std::endl;

        // For multiple flow specifications with flow deadlines within [deadline_min, deadline_max]
        auto& acceptance_ratios_map = exp1_design_given_topo_specs_deadline_range(single_topo_xml_path.c_str(), DEADLINE_MIN_SEC, DEADLINE_INCREMENT_SEC, DEADLINE_MAX_SEC, num_flows, output_dirname_fullpath, ds);
        

        for (auto it = acceptance_ratios_map.begin(); it != acceptance_ratios_map.end(); ++it) {
            std::cout << "Key: (" << format_time((it->first).first) <<","<< (it->first).second <<")"<<" -> Values: ";
            for (auto ar : it->second){
                std::cout <<ar.first <<","<<ar.second;
            }
            std::cout << "\n";
        }

        for (auto it = acceptance_ratios_map.begin(); it != acceptance_ratios_map.end(); ++it) {
            std::cout<<format_time((it->first).first)<<" "<<getAverageForKey(dc_acceptance_ratios, (it->first).first)<<" "
                <<getAverageForKey(rta_acceptance_ratios, (it->first).first)<<"\n";
        }

    }
    // Mode 2: Multiple Topologies in a single directory
    else if (multi_topology_mode){
        std::cout << "Processing multiple topologies in the directory: " << multi_topo_directory_path << std::endl;
        std::cout << "Number of flows: "<< num_flows <<std::endl;

        dir = opendir(multi_topo_directory_path.c_str());
        if (!dir){
            std::cerr << "Error: Could not open directory " << multi_topo_directory_path << std::endl;
            return 1;
        }

        // Put each topology in a XML file
        while((entry = readdir(dir)) != NULL){
            
            std::string fileName = entry->d_name;

            if (fileName == "." || fileName == "..")
                continue;

            // Filter only .xml files
            if (fileName.size() <= 4 || fileName.substr(fileName.size() - 4) != ".xml")
                continue;

            std::string fullPath = multi_topo_directory_path + fileName;
            std::cout<<"Analyzing Topology "<<fileName<<std::endl;

            // acceptance_ratio = design_given_topo_specs(fullPath.c_str(), num_flows, output_dirname_fullpath);
            // dc_acceptance_ratios.push_back(acceptance_ratio.first);
            // rta_acceptance_ratios.push_back(acceptance_ratio.second);

            acceptance_ratios_map = exp1_design_given_topo_specs_deadline_range(fullPath.c_str(), DEADLINE_MIN_SEC, DEADLINE_INCREMENT_SEC, DEADLINE_MAX_SEC, num_flows, output_dirname_fullpath, ds);
        }

        for (auto it = acceptance_ratios_map.begin(); it != acceptance_ratios_map.end(); ++it) {
            if (DBG_MAIN)
                std::cout << "Key: (" << format_time((it->first).first) <<","<< (it->first).second <<")"<<" -> Values: ";
            for (auto ar : it->second){
                if (DBG_MAIN)
                    std::cout <<ar.first <<","<<ar.second<<" ";
                dc_acceptance_ratios[(it->first).first].push_back(ar.first);
                rta_acceptance_ratios[(it->first).first].push_back(ar.second);
            }
            std::cout << "\n\n\n";
        }

        for (auto it = acceptance_ratios_map.begin(); it != acceptance_ratios_map.end(); ++it) {
            std::cout<<format_time((it->first).first)<<" "<<getAverageForKey(dc_acceptance_ratios, (it->first).first)<<" "
                <<getAverageForKey(rta_acceptance_ratios, (it->first).first)<<"\n";
        }

    }

    // Mode 3: Combined Network and Flow Specifications in a single XML file
    else if (full_info_mode){
        acceptance_ratio = design_given_topo_and_flow_specs(full_info_xml_path.c_str(), output_dirname_fullpath, ds);
    }

    return 0;


