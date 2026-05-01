#ifndef util_h
#define util_h

#include <random>
#include <mosek.h>
#include <unordered_map>
#include <queue>
#include <map>

#include "fusion.h"
using namespace mosek::fusion;
using namespace monty;

#define HIGH_PRIO 0
#define LOW_PRIO 7

//Elements of topo_factory
class Node;
class Link;
class Path;
//Elements of flow_factory
class Flow;
class AperiodicFlow;
//Elements of aperiodic_server
class AperiodicServer;
//Elements of system_factory
class RTNetView;
class RTNetwork;


typedef std::pair<Flow*, Node*> FlowNode;
typedef std::pair<Flow*, Link*> FlowLink;
typedef std::pair<Flow*, Path*> FlowPath;
typedef std::unordered_map<std::string, std::vector<uint>> usefulEdgesCountMap;
extern usefulEdgesCountMap usefulEdgesCtMap;

enum class flow_type {PERIODIC, APERIODIC};
enum class flow_safety_mode {PRIMARY, BACKUP};
//Delay Analysis Schemes
enum class latency_scheme {RTA, NC, DCT};

/* Number of Iterations*/
#define NUM_ITERATIONS 100

/*Contains general utility functions used by flow specifications, network specifications, and routing analysis */
int generate_random_seeded(int, int);
int generate_random_int(int, int);
double generate_random_double(double, double);
double generate_random_double_seeded(double, double);
int pick_rand_index_from_a_list(std::vector<double>& lst);
int pick_rand_index_from_a_short_list(std::vector<short>& lst);
std::string format_time(double);
std::string format_bandwidth(double);
void parseNetworkSpecifications(const char*, std::vector<Node*>&, std::vector<Link*>&);
void parseNetworkFlowSpecifications(const char*, std::vector<Node*>&, std::vector<Link*>&, std::vector<Flow*>&, std::vector<AperiodicFlow*>&);
void parseFlowSpecifications(const char*, std::vector<Flow*>&, std::vector<AperiodicFlow*>&);
std::vector<double> make_vector_with_step(double beg, double step, double end);
std::vector<short> make_short_vector_with_step(short beg, short step, short end);
//double getProcessingTime(const Flow& F, const Node& N);
bool isNumber(const char []);
std::pair<double, double> design_given_topo_and_flow_specs(const char*, std::string, latency_scheme);
std::pair<double, double> design_given_topo_specs(const char*, int, std::string, latency_scheme);
std::map<std::pair<double, int>, std::vector<std::pair<double, double>>>& exp1_design_given_topo_specs_deadline_range(const char*, double, double, double, int, std::string, latency_scheme);
double getAverageForKey(const std::map<double, std::vector<double>>&, double);
double getTotalForKey(const usefulEdgesCountMap&, std::string);
void filter_flow_stats(std::string, std::string, std::string);
double parse(std::string capacity);
double parse_time(std::string time_val);

void capture_all_interdependencies(std::vector<Node*>& node_specifications, 
    std::vector<Link*>& link_specifications, 
    std::vector<Flow*>& flow_specifications,
    std::vector<AperiodicFlow*>& aflow_specifications);

void capture_node_link_interdependencies(std::vector<Node*>& node_specifications, 
    std::vector<Link*>& link_specifications);

void addNode(Node *x, Path* P);
void addLink(Link *y, Path* P);


bool solve_server_constraints_contego(std::vector<double>& ServerParams, 
    double aperiodic_flow_period, uint server_prio, 
    RTNetView* aperiodic_topo_view, Node* N);

void logsumexp_server_util(Model::t, std::shared_ptr<ndarray<double, 2>>,
    Variable::t, std::shared_ptr<ndarray<double, 1>>);

#endif