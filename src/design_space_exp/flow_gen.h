#ifndef flow_gen_h
#define flow_gen_h

#include <design_space_exp.h>
#include <flow_factory.h>
#include <topo_factory.h>
#include <util.h>
#include <eval_parameters.h>



/*Class containing methods to generate flows in various ways*/
class FlowGenerator{
    public:
        int num_flows;
        NetworkGenerator *NG;

        //Flow Specifications
        std::vector<Flow*> generated_periodic_flows;
        std::vector<AperiodicFlow*> generated_aperiodic_flows;
        
        //Methods to generate flows
        FlowGenerator(std::vector<Flow*> flow_specifications, std::vector<AperiodicFlow*> aflow_specifications);
        FlowGenerator(int nr_flows);
        FlowGenerator(int nr_flows, NetworkGenerator* network_generator);  
        std::vector<Flow*> generate_periodic_flows_across_deadline_range_use_node_refs (double deadline_begin, double deadline_end);
        std::vector<Flow*> generate_periodic_flows_across_deadline_range_use_node_names(double deadline_begin, double deadline_end, int num_nodes);
        std::vector<Flow*> generate_periodic_flows_specific_deadline_use_node_names(double deadline, int num_nodes);

        void printGeneratedFlows();
};

#endif