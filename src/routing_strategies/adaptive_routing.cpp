#include <flow_factory.h>
#include <topo_factory.h>
#include <netdesign_factory.h>
#include <util.h>
#include <adaptive_routing.h>
#include <delay_model.h>
#include <unordered_map>
#include <vector>
#include <list>
#include <algorithm>
#include <netview_factory.h>
#include <debug_config.h>
#include <iomanip>

class AdaptiveRoutingLabelComparator_Alpha {
    //bool reverse;
    public:
        //AdaptiveRoutingLabelComparator(const bool &revparam = true) { reverse = revparam; }
        bool operator() (const AdaptiveRoutingLabel* lhs, const AdaptiveRoutingLabel* rhs) const {
            return ( lhs->alpha >= rhs->alpha);
        }
};

class AdaptiveRoutingLabelComparator_Beta {
    //bool reverse;
    public:
        //AdaptiveRoutingLabelComparator(const bool &revparam = true) { reverse = revparam; }
        bool operator() (const AdaptiveRoutingLabel *lhs, const AdaptiveRoutingLabel *rhs) const {
            return ( lhs->beta >= rhs->beta);
        }
};

/* Adaptation of Dijkstra all-to-one shortest path
* this is used by Baruah's routing scheme
* At the end of Dijkstra, w(v) and alpha(v) will carry 
* the shortest path and the cost of the path to the destination
* Summary of strategy:
* 1) Prior to staring from source vertex, identify a path P from source to destination,
* 2) Set out from the source vertex to the destination vertex along P
* 3) Continue following P as long as actual delay does not exceed the typical delay,
* 4) If we reach the destination vertex, we are done
* 5) Otherwise, let (u, v) denote the first edge where the actual delay has exceeded
* the typical delay. Then, from v, we travel along a potentially different path
* W(v)
*/

/* Given a 'Flow', a 'RTNetView', it outputs the best 'Path'
* Make sure to use the cost metrics from the topology to calculate cost
*/

AdaptiveRoutingLabel::AdaptiveRoutingLabel(Flow* F, Node* N){
    cur_flow = F;
    cur_node = N;
}

AdaptiveRouting::AdaptiveRouting(RTNetView* rtnv){

    // Flow* cur_periodic_flow;
    // AperiodicFlow* cur_aperiodic_flow;

    rt_netview = rtnv;

    DM = new DelayModel(rt_netview);

    if (DBG_ADAP_ROU)
        std::cout<<"Number of nodes in RTNetView = "<<rt_netview->rtnetview_nodes.size()<<std::endl;

    if (DBG_ADAP_ROU)
        std::cout<<"Initializing Beta's"<<std::endl;
    /* 
    * $\beta[node]$ -> delay from source
    * $\beta[node]$ -> if node == source, it includes perNodeDelay at the node
    */        
    for (Node* x: rt_netview->rtnetview_nodes){
        if (x == rtnv->source)
            beta[x] = rtnv->cur_periodic_flow->perNodeDelay(x);
        else
            beta[x] = __DBL_MAX__;
    }

    if (DBG_ADAP_ROU)
        std::cout<<"Initializing Alpha's"<<std::endl;
    /* 
    * $\alpha[node]$ -> delay to destination
    * $\alpha[node]$ -> does not include node delay if node == destination
    */
    for (Node* x: rt_netview->rtnetview_nodes){   
        if (x == rtnv->destination)
            alpha[x] = 0;
        else
            alpha[x] = __DBL_MAX__;
    }

    if (DBG_ADAP_ROU)
        std::cout<<"Initializing Adaptive Routing Labels"<<std::endl;
    for (Node* x: rt_netview->rtnetview_nodes){
        AdaptiveRoutingLabel* ARL = new AdaptiveRoutingLabel(rt_netview->cur_periodic_flow, x);
        ARL->alpha = alpha[x];
        //ARL->W = new Path();
        ARL->beta = beta[x];
        //ARL->T = new Path();
        ARL_dict[x] = ARL;
    }

    if (DBG_NW_DESIGN){
        std::cout<<"\n\n===================STAGE 2===================="<<std::endl;
        std::cout<<"----PREPROCESSING USING DIJKSTRA----"<<std::endl;
        std::cout<<"=============================================="<<std::endl;
    }

    // Preprocessing updating alpha parameters for each of the nodes starting at the destination
    // Run Djikstra starting at the "destination of the flow"

    switch(rtnv->rt_network->delay_scheme){

        case latency_scheme::RTA: 
            preprocessing_via_dijkstra_rta();
            break;
        
        case latency_scheme::DCT:
            preprocessing_via_dijkstra_dct();
            break;
        
        case latency_scheme::NC:
             preprocessing_via_dijkstra_nc();
             break;
        

    }

    if (DBG_ADAP_ROU)
        print_routing_table();

    if (DBG_NW_DESIGN){
        std::cout<<"\n=============================================="<<std::endl;
        std::cout<<"----END OF PREPROCESSING USING DIJKSTRA----"<<std::endl;
        std::cout<<"=============================================="<<std::endl;
    }
}

// This function does preprocessing and stores a look-up table at every node
// The lookup table contains for every v:
// alpha[v] -- the min worst case cost to reach the destination, 
// W[v] -- the path that gives alpha[v] delay
// It does preprocessing using response-time analysis
void AdaptiveRouting::preprocessing_via_dijkstra_rta(){
    
    // Calculate $\alpha(v)$ & $W(v)$ using a shortest path algorithm 
    // Dijkstra's Single Source Shortest Path Algorithm
    // Perform the analysis per node

    std::unordered_map<Node*, bool, CustomNodePHash> visited;
    Node* dest_node; // dest_node is the destination of the Node* destination
    Node* accepted_node; //cur_node is the node from where we are exploring and it is in the cheapest path
    Node* cheapest_node; // cheapest node is the node that is cheapest in an exploration. Why do we need it? yes we need it, if we compute the alpha for a node in 2 hops, it needs to go via cheapest node in 1 hop
    Node* discovered_node; //nodes discovered in an exploration
    
    double alpha_val; //used to store new alpha[v]

    PriorityQueueCustom_Alpha PQA; // to select the cheapest node in a layer 
    uint ctr = 1; // keeping track of number of neighbours

    Path* accepted_path = new Path();
    Path* possible_path = new Path();
    // std::unordered_map<Node*, std::vector<Path*>, CustomNodePHash> candidate_paths;

    Path* cheapest_path;
    Link* lk;
    Node* z;

    // We create a label with the destination and push it to the priority queue
    dest_node = rt_netview->destination;
    // THINK: Should we create a dest_label with the destination and add it to the priority queue?
    // AdaptiveRoutingLabel* dest_label = new AdaptiveRoutingLabel(cur_flow, dest_node);
    // dest_label->alpha = 0;
    // dest_label->W = 0;
    // PQ.push(dest_label);

    //Add the destination to the accepted_path since it is guaranteed to be the cheapest
    addNode(dest_node, accepted_path);
    accepted_node = dest_node;
    visited[accepted_node] = false; // visited is marked as true when all the neighbours have been dicovered

    if (rt_netview->cur_periodic_flow){
        if (DBG_ADAP_ROU_PRE)
            std::cout<<"\n"<<__func__<<" : "<<"Preprocessing starting at the destination of Periodic Flow "<<rt_netview->cur_periodic_flow->flow_id<<" i.e., "<<accepted_node->node_name<<std::endl;
    }
    else{
        if (DBG_ADAP_ROU_PRE)
            std::cout<<"\n"<<__func__<<" : "<<"Preprocessing starting at the destination of Aperiodic Flow "<<rt_netview->cur_aperiodic_flow->flow_id<<" i.e., "<<accepted_node->node_name<<std::endl;
    }
    while((accepted_node != rt_netview->source) && (!visited[accepted_node])){
        
        //accepted_node = PQ.top()->cur_node;
        //PQ.pop();
        if(DBG_ADAP_ROU_PRE){
            std::cout<<"\n\tAccepted Path before going through all the neighbours of "<<accepted_node->node_name<<" = ";
            rt_netview->printPath(accepted_path);
            std::cout<<"\n";
        }

        // Go through all the adjacent links
        for (auto discovered_link:accepted_node->links){
            
            if (DBG_ADAP_ROU_PRE)
                std::cout<<"\n\tIteration = "<<ctr++<<"/"<<accepted_node->links.size()<<" : ";
            
            // Only consider links that are part of the topo_view
            if(std::find(rt_netview->rtnetview_links.begin(), rt_netview->rtnetview_links.end(), discovered_link)!=rt_netview->rtnetview_links.end()){
                
                discovered_node = (discovered_link->node1 == accepted_node) ? discovered_link->node2 : discovered_link->node1;
            
                if (DBG_ADAP_ROU_PRE)
                    std::cout<<"Discovered Link "<<accepted_node->node_name<<"----"<<discovered_node->node_name<<" in rt_netview"<<"\n\n";

                if (!visited[discovered_node]){
            
                    //define possible_path starting from the accepted_path everytime
                    possible_path = new Path(accepted_path->nodes, accepted_path->links);
                    addLink(discovered_link, possible_path);
                    addNode(discovered_node, possible_path);

                    if (DBG_ADAP_ROU_PRE){
                        std::cout<<"\t\tEvaluating Possible Path = ";
                        rt_netview->printPath(possible_path);
                        std::cout<<"\n";
                    }          

                    //Calculate possible alpha[discovered_node] -- we use RTA here since alpha[v] aims to capture worst-case delay to destination
                    if (rt_netview->cur_periodic_flow){
                        // Response-Time Analysis
                        alpha_val = alpha[accepted_node] + rt_netview->cur_periodic_flow->perLinkDelay(discovered_link) + rt_netview->cur_periodic_flow->perNodeDelay(discovered_node);

                        if (DBG_ADAP_ROU_PRE){
                            std::cout<<"\t\t"<<"\talpha["<<discovered_node->node_name<<"] = "
                                <<format_time(alpha_val)<<" = "<<format_time(alpha[accepted_node])<<"(alpha["<<accepted_node->node_name<<"])"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perLinkDelay(discovered_link))<<"(Link Delay("<<discovered_link->link_name<<"))"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perNodeDelay(discovered_node))<<"(Node Delay("<<discovered_node->node_name<<"))"
                                <<"\n";

                            std::cout<<"\t\t"<<"\tRTA Delay = "<<format_time(alpha_val)<<"\tDCT Delay = "<<format_time(DM->getPathDelay(*(rt_netview->cur_periodic_flow), *(possible_path)))<<std::endl;
                            std::cout<<"\t\t"<<"\tFinal Delay (Choosen by RTA) = "<<format_time(alpha_val)<< " along ";
                            rt_netview->printPath(possible_path);
                            std::cout<<"\n";
                        }
                    }
                    // In case we use Adaptive Routing for Aperiodic Flows 
                    else{
                        // Response-Time Analysis
                        alpha_val = alpha[accepted_node] + rt_netview->cur_aperiodic_flow->perLinkDelay(discovered_link) + rt_netview->cur_aperiodic_flow->perNodeDelay(discovered_node);

                        if (DBG_ADAP_ROU_PRE){
                            std::cout<<"\t\t"<<"\talpha["<<discovered_node->node_name<<"] = "
                                <<format_time(alpha_val)<<" = "<<format_time(alpha[accepted_node])<<"(alpha["<<accepted_node->node_name<<"])"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perLinkDelay(discovered_link))<<"(Link Delay("<<discovered_link->link_name<<"))"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perNodeDelay(discovered_node))<<"(Node Delay("<<discovered_node->node_name<<"))"
                                <<"\n";                        

                            std::cout<<"\t\t"<<"\tRTA Delay = "<<format_time(alpha_val)<<"\tDCT Delay = "<<format_time(DM->getPathDelay(*(rt_netview->cur_aperiodic_flow), *(possible_path)))<<std::endl;
                            std::cout<<"\t\t"<<"\tFinal Delay (Choosen by RTA) = "<<format_time(alpha_val)<< " along ";
                            rt_netview->printPath(possible_path);
                            std::cout<<"\n";
                        }
                    }
                    //Update delays and possible_path if it is better than previous iterations
                    if (DBG_ADAP_ROU_PRE){
                        std::cout<<"\t\t"<<"\tCondition Check : Old alpha["<<discovered_node->node_name<<"] = "<<format_time(alpha[discovered_node])
                            <<"\tNew alpha["<<discovered_node->node_name<<"] = "<<format_time(alpha_val)<<std::endl;
                    }
                    // If better alpha_val is found, update the alpha[.] for the discovered_node
                    if (alpha_val <= alpha[discovered_node]){
                        alpha[discovered_node] = alpha_val;
                        W[discovered_node] = possible_path;
                        if (DBG_ADAP_ROU_PRE)
                            std::cout<<"\t\t**Delay to Destination (alpha["<<discovered_node->node_name<<"]) & Path to Destination W("<<discovered_node->node_name<<") updated via RTA** \n";
                        ARL_dict[discovered_node]->alpha = alpha[discovered_node];
                        ARL_dict[discovered_node]->W = W[discovered_node];
                        // Add the node's label to the priority queue if not previously discovered coz otherwise its there in PQ
                        PQA.push(ARL_dict[discovered_node]);
                    }
                }
                else{
                    if(DBG_ADAP_ROU_PRE)
                        std::cout<<"\t\tSkipping Possible Path = "<<discovered_node->node_name<<"----"<<discovered_link->link_name<<"----"<<accepted_node->node_name<<"\n";
                }   
            }// done going through one link
            else{
                if (DBG_ADAP_ROU_PRE)
                    std::cout<<"Discovered Link "<<discovered_link->link_name<<" not in rt_netview"<<"\n\n";
            }
        }// done going through all the adjacent links
        
        if (DBG_ADAP_ROU_PRE)
            std::cout<<"\n\tDone going through all links of "<<accepted_node->node_name<<std::endl;

        // Go through all the labels in PriorityQueueCustom_Alpha to update the accepted_path
        // Why do we need the accepted_path? we need to track the lowest cost node in every layer so that we can use the alpha[accepted_node]
        // to update the alpha[discovered_node].
        visited[accepted_node] = true ;

        //Get the node minimizing the cost so that the next layer of nodes can use this
        if(!PQA.empty()){
            //Get the cheapest_node & path to the cheapest_node
            cheapest_node = PQA.top()->cur_node;
            cheapest_path = PQA.top()->W;
            if (DBG_ADAP_ROU_PRE){
                std::cout<<"\t\tCheapest Node connected to "<<accepted_node->node_name<<" = "<<cheapest_node->node_name<<" with alpha["<<cheapest_node->node_name<<"] = "<<format_time(alpha[cheapest_node])<<"\n";
                std::cout<<"\t\tCheapest Path = ";
                rt_netview->printPath(cheapest_path);
                std::cout<<"\n";
            }
            addNode(cheapest_node, accepted_path);
            accepted_node = cheapest_node;
            accepted_path = cheapest_path;
            PQA.pop();
        }

        // Commented this since what if there is a node attached to the second-cheapest node?
        // Clean up other links and nodes
        // i.e., until PQA is empty
        // while(!PQA.empty()){
        //     z = PQA.top()->cur_node;
        //     //TODO: Should we reset the visited to false??
        //     //visited[z] = false;
        //     if (DBG_ADAP_ROU)
        //         std::cout<<"alpha["<<z->node_name<<"] = "<<alpha[z]<<"\t";
        //     PQ.pop();
        // }

        if (DBG_ADAP_ROU_PRE){
            std::cout<<"\n\tAccepted Path after going through all the neighbours  = ";
            rt_netview->printPath(accepted_path);
            std::cout<<"\n";
        }
        ctr = 1;
    }

}

// This function does preprocessing and stores a look-up table at every node
// The lookup table contains for every v:
// alpha[v] -- the min worst case cost to reach the destination, 
// W[v] -- the path that gives alpha[v] delay
// It does preprocessing using delay composition theorem
void AdaptiveRouting::preprocessing_via_dijkstra_dct(){
    
    // Calculate $\alpha(v)$ & $W(v)$ using a shortest path algorithm 
    // Dijkstra's Single Source Shortest Path Algorithm
    // Perform the analysis per node

    std::unordered_map<Node*, bool, CustomNodePHash> visited;
    Node* dest_node; // dest_node is the destination of the Node* destination
    Node* accepted_node; //cur_node is the node from where we are exploring and it is in the cheapest path
    Node* cheapest_node; // cheapest node is the node that is cheapest in an exploration. Why do we need it? yes we need it, if we compute the alpha for a node in 2 hops, it needs to go via cheapest node in 1 hop
    Node* discovered_node; //nodes discovered in an exploration
    
    double alpha_val; //used to store new alpha[v]

    PriorityQueueCustom_Alpha PQA; // to select the cheapest node in a layer 
    uint ctr = 1; // keeping track of number of neighbours

    Path* accepted_path = new Path();
    Path* possible_path = new Path();
    // std::unordered_map<Node*, std::vector<Path*>, CustomNodePHash> candidate_paths;

    Path* cheapest_path;
    Link* lk;
    Node* z;

    // We create a label with the destination and push it to the priority queue
    dest_node = rt_netview->destination;
    // THINK: Should we create a dest_label with the destination and add it to the priority queue?
    // AdaptiveRoutingLabel* dest_label = new AdaptiveRoutingLabel(cur_flow, dest_node);
    // dest_label->alpha = 0;
    // dest_label->W = 0;
    // PQ.push(dest_label);

    //Add the destination to the accepted_path since it is guaranteed to be the cheapest
    addNode(dest_node, accepted_path);
    accepted_node = dest_node;
    visited[accepted_node] = false; // visited is marked as true when all the neighbours have been dicovered

    if (rt_netview->cur_periodic_flow){
        if (DBG_ADAP_ROU_PRE)
            std::cout<<"\n"<<__func__<<" : "<<"Preprocessing starting at the destination of Periodic Flow "<<rt_netview->cur_periodic_flow->flow_id<<" i.e., "<<accepted_node->node_name<<std::endl;
    }
    else{
        if (DBG_ADAP_ROU_PRE)
            std::cout<<"\n"<<__func__<<" : "<<"Preprocessing starting at the destination of Aperiodic Flow "<<rt_netview->cur_aperiodic_flow->flow_id<<" i.e., "<<accepted_node->node_name<<std::endl;
    }
    while((accepted_node != rt_netview->source) && (!visited[accepted_node])){
        
        //accepted_node = PQ.top()->cur_node;
        //PQ.pop();
        if(DBG_ADAP_ROU_PRE){
            std::cout<<"\n\tAccepted Path before going through all the neighbours of "<<accepted_node->node_name<<" = ";
            rt_netview->printPath(accepted_path);
            std::cout<<"\n";
        }

        // Go through all the adjacent links
        for (auto discovered_link:accepted_node->links){
            
            if (DBG_ADAP_ROU_PRE)
                std::cout<<"\n\tIteration = "<<ctr++<<"/"<<accepted_node->links.size()<<" : ";
            
            // Only consider links that are part of the topo_view
            if(std::find(rt_netview->rtnetview_links.begin(), rt_netview->rtnetview_links.end(), discovered_link)!=rt_netview->rtnetview_links.end()){
                
                discovered_node = (discovered_link->node1 == accepted_node) ? discovered_link->node2 : discovered_link->node1;
            
                if (DBG_ADAP_ROU_PRE)
                    std::cout<<"Discovered Link "<<accepted_node->node_name<<"----"<<discovered_node->node_name<<" in rt_netview"<<"\n\n";

                if (!visited[discovered_node]){
            
                    //define possible_path starting from the accepted_path everytime
                    possible_path = new Path(accepted_path->nodes, accepted_path->links);
                    addLink(discovered_link, possible_path);
                    addNode(discovered_node, possible_path);

                    if (DBG_ADAP_ROU_PRE){
                        std::cout<<"\t\tEvaluating Possible Path = ";
                        rt_netview->printPath(possible_path);
                        std::cout<<"\n";
                    }          

                    //Calculate possible alpha[discovered_node] -- we use RTA here since alpha[v] aims to capture worst-case delay to destination
                    if (rt_netview->cur_periodic_flow){
                        // Delay Composition Theorem
                        alpha_val = DM->getPathDelay(*(rt_netview->cur_periodic_flow), *(possible_path));

                        if (DBG_ADAP_ROU_PRE){
                            std::cout<<"\t\t"<<"\talpha["<<discovered_node->node_name<<"] = "
                                <<format_time(alpha_val)<<" = "<<format_time(alpha[accepted_node])<<"(alpha["<<accepted_node->node_name<<"])"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perLinkDelay(discovered_link))<<"(Link Delay("<<discovered_link->link_name<<"))"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perNodeDelay(discovered_node))<<"(Node Delay("<<discovered_node->node_name<<"))"
                                <<"\n";

                            std::cout<<"\t\t"<<"\tRTA Delay = "<<format_time(alpha[accepted_node] + rt_netview->cur_periodic_flow->perLinkDelay(discovered_link) + rt_netview->cur_periodic_flow->perNodeDelay(discovered_node))
                                <<"\tDCT Delay = "<<format_time(alpha_val)<<std::endl;
                            std::cout<<"\t\t"<<"\tFinal Delay (Choosen by DCT) = "<<format_time(alpha_val)<< " along ";
                            rt_netview->printPath(possible_path);
                            std::cout<<"\n";
                        }
                    }
                    // In case we use Adaptive Routing for Aperiodic Flows 
                    else{
                        // Delay Composition Theorem
                        alpha_val = DM->getPathDelay(*(rt_netview->cur_aperiodic_flow), *(possible_path));

                        if (DBG_ADAP_ROU_PRE){
                            std::cout<<"\t\t"<<"\talpha["<<discovered_node->node_name<<"] = "
                                <<format_time(alpha_val)<<" = "<<format_time(alpha[accepted_node])<<"(alpha["<<accepted_node->node_name<<"])"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perLinkDelay(discovered_link))<<"(Link Delay("<<discovered_link->link_name<<"))"
                                <<" + "<<format_time(rt_netview->cur_periodic_flow->perNodeDelay(discovered_node))<<"(Node Delay("<<discovered_node->node_name<<"))"
                                <<"\n";                        

                            std::cout<<"\t\t"<<"\tRTA Delay = "<<format_time(alpha[accepted_node] + rt_netview->cur_aperiodic_flow->perLinkDelay(discovered_link) + rt_netview->cur_aperiodic_flow->perNodeDelay(discovered_node))
                                <<"\tDCT Delay = "<<format_time(alpha_val)<<std::endl;
                            std::cout<<"\t\t"<<"\tFinal Delay (Choosen by DCT) = "<<format_time(alpha_val)<< " along ";
                            rt_netview->printPath(possible_path);
                            std::cout<<"\n";
                        }
                    }
                    //Update delays and possible_path if it is better than previous iterations
                    if (DBG_ADAP_ROU_PRE){
                        std::cout<<"\t\t"<<"\tOld alpha["<<discovered_node->node_name<<"] = "<<format_time(alpha[discovered_node])
                            <<"\tNew alpha["<<discovered_node->node_name<<"] = "<<format_time(alpha_val)<<std::endl;
                    }
                    // If better alpha_val is found, update the alpha[.] for the discovered_node
                    if (alpha_val <= alpha[discovered_node]){
                        alpha[discovered_node] = alpha_val;
                        W[discovered_node] = possible_path;
                        if (DBG_ADAP_ROU_PRE)
                            std::cout<<"\t\t**Delay to Destination (alpha["<<discovered_node->node_name<<"]) & Path to Destination W("<<discovered_node->node_name<<") updated** \n";
                        ARL_dict[discovered_node]->alpha = alpha[discovered_node];
                        ARL_dict[discovered_node]->W = W[discovered_node];
                        // Add the node's label to the priority queue if not previously discovered coz otherwise its there in PQ
                        PQA.push(ARL_dict[discovered_node]);
                    }
                }
                else{
                    if(DBG_ADAP_ROU_PRE)
                        std::cout<<"\t\tSkipping Possible Path = "<<discovered_node->node_name<<"----"<<discovered_link->link_name<<"----"<<accepted_node->node_name<<"\n";
                }   
            }// done going through one link
            else{
                if (DBG_ADAP_ROU_PRE)
                    std::cout<<"Discovered Link "<<discovered_link->link_name<<" not in rt_netview"<<"\n\n";
            }
        }// done going through all the adjacent links
        
        if (DBG_ADAP_ROU_PRE)
            std::cout<<"\n\tDone going through all links of "<<accepted_node->node_name<<std::endl;

        // Go through all the labels in PriorityQueueCustom_Alpha to update the accepted_path
        // Why do we need the accepted_path? we need to track the lowest cost node in every layer so that we can use the alpha[accepted_node]
        // to update the alpha[discovered_node].
        visited[accepted_node] = true ;

        //Get the node minimizing the cost so that the next layer of nodes can use this
        if(!PQA.empty()){
            //Get the cheapest_node & path to the cheapest_node
            cheapest_node = PQA.top()->cur_node;
            cheapest_path = PQA.top()->W;
            if (DBG_ADAP_ROU_PRE){
                std::cout<<"\t\tCheapest Node connected to "<<accepted_node->node_name<<" = "<<cheapest_node->node_name<<" with alpha["<<cheapest_node->node_name<<"] = "<<format_time(alpha[cheapest_node])<<"\n";
                std::cout<<"\t\tCheapest Path = ";
                rt_netview->printPath(cheapest_path);
                std::cout<<"\n";
            }
            addNode(cheapest_node, accepted_path);
            accepted_node = cheapest_node;
            accepted_path = cheapest_path;
            PQA.pop();
        }

        // Commented this since what if there is a node attached to the second-cheapest node?
        // Clean up other links and nodes
        // i.e., until PQA is empty
        // while(!PQA.empty()){
        //     z = PQA.top()->cur_node;
        //     //TODO: Should we reset the visited to false??
        //     //visited[z] = false;
        //     if (DBG_ADAP_ROU)
        //         std::cout<<"alpha["<<z->node_name<<"] = "<<alpha[z]<<"\t";
        //     PQ.pop();
        // }

        if (DBG_ADAP_ROU_PRE){
            std::cout<<"\n\tAccepted Path after going through all the neighbours  = ";
            rt_netview->printPath(accepted_path);
            std::cout<<"\n";
        }
        ctr = 1;
    }

}


// This function does preprocessing and stores a look-up table at every node
// The lookup table contains for every v:
// alpha[v] -- the min worst case cost to reach the destination, 
// W[v] -- the path that gives alpha[v] delay
// It does preprocessing using network calculus
void AdaptiveRouting::preprocessing_via_dijkstra_nc(){

}

// This function prints the look-up table at every node that has been populated 
// by preprocessing_via_dijkstra()
// The lookup table contains for every v:
// alpha[v] -- the min worst case cost to reach the destination, 
// W[v] -- the path that gives alpha[v] delay
// beta[v] -- the actual delay consumed from source
// T[v] -- the path that gives beta[v] delay
void AdaptiveRouting::print_routing_table(){

    std::cout<<"\n\n=============================================="<<std::endl;
    std::cout<<"****************ROUTING TABLE************"<<std::endl;
    std::cout<<"=============================================="<<std::endl;

    std::cout<<std::setw(20)<<std::left<<"Node Name"
        <<std::setw(20)<<std::left<<"Beta"
        // <<std::setw(20)<<std::left<<"T"
        <<std::setw(20)<<std::left<<"Alpha"
        // <<std::setw(20)<<std::left<<"W"<<std::endl;
        <<std::endl;
    
    for (Node* x: rt_netview->rtnetview_nodes){
        std::cout<<std::setw(20)<<std::left<<x->node_name
            <<std::setw(20)<<std::left<<format_time(beta[x])
            <<std::setw(20)<<std::left<<format_time(alpha[x])<<"\n";
            // rt_netview->printPath(T[x]);
            // rt_netview->printPath(W[x]);
    }
    std::cout<<"==============================================";
    std::cout<<std::endl<<std::endl;
}

bool AdaptiveRouting::semi_adaptive_realtime_routing_dct(RTNetView* rt_nv, bool EMPTY_NETWORK, bool IS_APERIODIC=false){
    // Cost metrics from the graph
    DelayModel *DM = new DelayModel(rt_nv);
    PriorityQueueCustom_Beta PQB;
    Node* u;
    Node* v;

    uint count_useful = 0;

    Node* lowest_cost_link;
    Node* cur_node;

    std::unordered_map<Node*, bool, CustomNodePHash> visited;

    Path* result_path = new Path();
    Path* useful_path = new Path();
    Path* possibly_useful_path = new Path();
    Path* temp_path = new Path();

    Path* P_min_rta;
    Path* P_min_frc;

    Flow* F;
    AperiodicFlow* AF;
    double scale;

    if (IS_APERIODIC)
        AF = rt_nv->cur_aperiodic_flow;
    else
        F = rt_nv->cur_periodic_flow;
    
    // Record the shortest and longest delays from every node to the destination across all the paths
    if (DBG_ADAP_ROU){
        //std::cout<<"\n\n\t\t========================================================"<<std::endl;
        std::cout<<"\n\n"<<std::string(240, '=')<<std::endl;
        std::cout<<"\t\t"<<"***********"<<"EXPECTED DELAY PREPARATION "<<rt_netview->cur_periodic_flow->flow_name<<"************"<<std::endl;
        std::cout<<std::string(240, '=')<<std::endl;
        //std::cout<<"\t\t========================================================"<<std::endl;

        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        std::cout<<"\t\t"<<std::setw(20)<<std::left<<"Path Length"
            <<std::setw(20)<<std::left<<"Exp. Delay (RTA)"
            <<std::setw(20)<<std::left<<"Exp. Delay (FRC)"
            <<std::setw(30)<<std::left<<"Possible Min Path Delay(RTA)"<<"\t"
            <<std::setw(30)<<std::left<<"Possible Min Path Delay(FRC)"<<std::endl;
        //std::cout<<"\t\tFlow"<<"\t\t"<<"Source"<<"\t\t"<<"Destination"<<"\t"<<"Best Delay"<<"\t"<<"Worst Delay"<<std::endl;
        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        //std::cout<<"\t\t=================================================================================="<<std::endl;
    }
    double rta_val, frc_val;


    for (auto P: rt_nv->all_possible_paths){
        if (IS_APERIODIC){
            rta_val = AF->getPathDelay(P);
            frc_val = DM->getPathDelay(*AF,*P);

        }
        else{
            rta_val = F->getPathDelay(P);
            frc_val = DM->getPathDelay(*F,*P);
        }
        if (rta_val <= F->expected_min_delay_rta){
                F->expected_min_delay_rta = rta_val;
                P_min_rta = P;
        }
        if (frc_val <= F->expected_min_delay_frc){
                F->expected_min_delay_frc = frc_val;
                P_min_frc = P;
        }

        if (DBG_ADAP_ROU){
            std::cout<<"\t\t"<<std::setw(20)<<std::left<<(P->links).size()
                <<std::setw(20)<<std::left<<format_time(rta_val)
                <<std::setw(20)<<std::left<<format_time(frc_val);
            //std::cout<<std::setw(20);
            std::cout<<std::left;
            rt_nv->printPath(P_min_rta);
            std::cout<<"\t\t"<<std::right;
            rt_nv->printPath(P_min_frc);
            std::cout<<std::endl;
        }
    }

    if (DBG_ADAP_ROU){
        std::cout<<"\n";
        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        //std::cout<<"\n\t\t========================================================"<<std::endl;
        std::cout<<"\t\t***********END OF DELAY PREPARATION "<<rt_netview->cur_periodic_flow->flow_name<<"************"<<std::endl;
        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        //std::cout<<"\t\t========================================================"<<std::endl;
    }


    if((EMPTY_NETWORK) || (IS_APERIODIC)){
        if (DBG_ADAP_ROU)
            std::cout<<"\n\tAssigning Path using Dijkstra's Shortest Path (Number of Hops)\n";
        result_path = rt_nv->get_shortest_hoplength_path();

        if (IS_APERIODIC)
            AF->assigned_path = result_path;
        else
            F->assigned_path = result_path;
    }

    else {

        //Add all the vertices to the priority queue
        // for (Node* x: A->netview_nodes){
        //     AdaptiveRoutingLabel* ARx = new AdaptiveRoutingLabel(F, x);
        //     ARx->beta = beta[x];
        //     PQ.push(ARx);
        // }

        for (Node* x: rt_nv->rtnetview_nodes){
            visited[x] = false;
        }


        //Add only the source node to the queue
        cur_node = F->source;
        addNode(cur_node, useful_path);

        /* AdaptiveRoutingLabel* ARL_src = new AdaptiveRoutingLabel(F, cur_node);
        ARL_src->beta = beta[cur_node];
        PQ.push(ARL_src);*/

        if (DBG_ADAP_ROU)
            std::cout<<"\n\tStarting at "<<cur_node->get_name()<<std::endl;

        while(cur_node != F->destination){
            //u = PQ.top()->cur_node;
            //PQ.pop();
            //result_path->nodes.push_back(u);

            //exploring the neighbours here
            for (auto l: cur_node->links){
                //reset the possibly_useful_path to useful_path before processing each link     
                possibly_useful_path = new Path(useful_path->nodes, useful_path->links);

                //Process this link
                if(std::find(rt_nv->rtnetview_links.begin(), rt_nv->rtnetview_links.end(), l) != rt_nv->rtnetview_links.end()){
                    v = (l->node1 == cur_node) ? l->node2 : l->node1;
                    
                    if (DBG_ADAP_ROU)
                        std::cout<<"\n\n\t\tSemi-Adaptive Routing: Discovered Link "<<cur_node->node_name<<"----"<<v->get_name()<<"\n";

                    // Edge-Relaxation -- Check if the edge is useful
                    // if ((beta[x]+Cw(x,y)+alpha[y] <= D) && (beta[x] + Ct(x,y) <= beta[y])){
                    
                    // ISSUE: Double counting F->perNodeDelay(v) in both F->perNodeDelay(v) and alpha[v]?
                    /* if ((beta[cur_node] + F->perNodeDelay(cur_node) + F->perLinkDelay(l) + F->perNodeDelay(v) + alpha[v] <= F->relative_deadline) &&
                    *     (beta[cur_node] + F->perNodeDelay(cur_node) + F->perLinkDelay(l) + F->perNodeDelay(v) <= beta[v])){
                    */

                    // Option 1: Define a useful edge condition based on Delay Composition Theorem
                    temp_path = new Path(possibly_useful_path->nodes, possibly_useful_path->links);
                    addNode(v, temp_path);
                    addLink(l, temp_path);
                    //scale = generate_random_double_seeded(0.0, 1.0);
                    scale = 1.0;
                    double updated_end_to_end_delay = (scale * DM->getPathDelay(*(F), *(temp_path))) + (alpha[v]-rt_nv->node_view[v]);//- rt_nv->node_view[v];
                    double updated_beta = (scale * DM->getPathDelay(*(F), *(temp_path)));
                    if (DBG_ADAP_ROU){
                        std::cout<<"\tConsidering Path = ";
                        temp_path->printPath();
                        std::cout<<std::endl;
                        std::cout<<__LINE__<<"\t\tUpdated End to End Delay = "<<format_time(updated_end_to_end_delay)<<"\t"
                            <<"Updated Beta = "<<format_time(updated_beta)<<"\n";
                    }
                    if((updated_end_to_end_delay <= F->relative_deadline) && (updated_beta <= beta[v]) && (!visited[v])){
                        
                    //Option 2: Define a useful edge condition based on the expression from Adaptive Routing paper
                    
                    // if ((beta[cur_node] + rt_nv->link_view[l] + alpha[v] <= F->relative_deadline) &&
                    //     (beta[cur_node] + rt_nv->link_view[l] + rt_nv->node_view[v] <= beta[v])){

                    //Option 3: Use RTA-based Delays
                    // if (DBG_ADAP_ROU){
                    //     std::cout<<"\t\t\tAdaptive Routing Condition Check: "<<"\n\t\t\t 1. beta["<<cur_node->node_name<<"]="<<format_time(beta[cur_node])<<", "<<"F->perLinkDelay("<<l->link_name<<")="<<format_time(F->perLinkDelay(l))<<", ";
                    //     std::cout<<"alpha["<<v->get_name()<<"]="<<format_time(alpha[v])<<", "<<"Relative Deadline = "<<format_time(F->relative_deadline)<<std::endl;
                    //     std::cout<<"\t\t\t 2. beta["<<cur_node->get_name()<<"] + F->perLinkDelay("<<l->link_name<<") + F->perNodeDelay("<<v->node_name<<") = "<<format_time(beta[cur_node] + F->perLinkDelay(l) + F->perNodeDelay(v))<<" Vs."<<"beta[v] = "<<format_time(beta[v])<<std::endl;
                    //     std::cout<<"\t\t\t 3. visited["<<v->get_name()<<"] = "<<visited[v]<<std::endl;
                    // }
                    // if ((beta[cur_node] + F->perLinkDelay(l) + alpha[v] <= F->relative_deadline) &&
                    //     (beta[cur_node] + F->perLinkDelay(l) + F->perNodeDelay(v) <= beta[v]) && !visited[v]){               
                        if (DBG_ADAP_ROU){
                            DBG_PRINT
                        }
                        // addNode(v, possibly_useful_path);
                        // addLink(l, possibly_useful_path);
                        count_useful = count_useful + 1;
                        
                        // Option 1:
                        beta[v] = updated_beta;

                        //Option 2:
                        //beta[v] = beta[cur_node] + rt_nv->node_view[v] + F->perLinkDelay(l);
                        //Option 3: 
                        //beta[v] = beta[cur_node] + F->perLinkDelay(l) + F->perNodeDelay(v);

                        
                        if (DBG_ADAP_ROU)
                            std::cout<<"\t\t"<<cur_node->node_name<<"----"<<v->get_name()<<" "<<"Useful Edge with RTA DIST(.) = "<<format_time(beta[cur_node] + rt_nv->node_view[v] + F->perLinkDelay(l))<<"\tDCT DIST(.) = "<<format_time(updated_beta)<<std::endl;
                        
                        // beta[v] = CM->getPathDelay(*(F), *(possibly_useful_path));
                        // std::cout<<"Useful Edge with DIST(.) = "<<beta[v]<<std::endl;
                        // result_path->nodes.push_back(v);
                        // result_path->links.push_back(l);

                        AdaptiveRoutingLabel* ARv = new AdaptiveRoutingLabel(F, v);
                        ARv->beta = beta[v];
                        ARv->T = temp_path; //possibly_useful_path;
                        PQB.push(ARv);
                    }
                    else{
                        if (DBG_ADAP_ROU){
                            //std::cout<<"\t\t"<<cur_node->node_name<<"----"<<v->get_name()<<" "<<"Not Useful Edge as it would give end-to-end delay = "<<format_time(beta[cur_node] + F->perLinkDelay(l) + alpha[v])<<std::endl;
                            std::cout<<"\t\t"<<cur_node->node_name<<"----"<<v->get_name()<<" "<<"Not Useful Edge as it would give end-to-end delay = "<<format_time(updated_end_to_end_delay)<<std::endl;
                        }
                    }
                }//done with one link
            
            }//done with all the links on cur_node

            if (DBG_ADAP_ROU)
                std::cout<<"\n\tDone with all the links of "<<cur_node->get_name()<<std::endl;
            
            visited[cur_node] = true;

            if (PQB.empty()){
                if (DBG_ADAP_ROU)
                    std::cout<<"\t\tNo useful edges from "<<cur_node->get_name()<<"-- No Path, going back"<<std::endl;
                // Retrace back to the prev cur_node
                if(useful_path->nodes.size() >= 2){
                    useful_path->nodes.pop_back();
                    useful_path->links.pop_back();
                    cur_node = useful_path->nodes.back();
                }
                else
                    return false;
            }
            //Finding lowest cost path
            else{
                AdaptiveRoutingLabel* AR_lowest_cost = PQB.top();
                if (!AR_lowest_cost){
                    if (DBG_ADAP_ROU)
                        std::cout<<"\t\tAR_lowest_cost not found\n";
                }
                else{
                    if (DBG_ADAP_ROU){
                        std::cout<<"\t\t=============================================="<<std::endl;
                        std::cout<<"\t\tPicked Link "<<cur_node->node_name<<"----"<<AR_lowest_cost->cur_node->get_name()<<"\n";
                    }
                    cur_node = AR_lowest_cost->cur_node;
                    useful_path = AR_lowest_cost->T;
                    if (DBG_ADAP_ROU){
                        std::cout<<"\t\tLowest Cost Next Node : "<<cur_node->get_name()<<"\t";
                        rt_netview->printPath(useful_path);
                        std::cout<<"\n";
                        std::cout<<"\t\t=============================================="<<std::endl;
                    }
                    PQB.pop();
                }
                //Clean up other links and nodes
                //i.e., until PQ is empty
                while(!PQB.empty())
                    PQB.pop();
            }
            if (DBG_ADAP_ROU)
                print_routing_table();
        } // goto the next cur_node and loop until the destination is reached 

        result_path = useful_path;
        F->assigned_path = result_path;
    } // Path Assignment complete

    if (DBG_ADAP_ROU){
        std::cout<<"\n\tSemi-Adaptive Routing Path is : ";
        if (IS_APERIODIC){
            rt_nv->printPath(AF->assigned_path);
            std::cout<<"\n";
        }
        else{
            rt_nv->printPath(F->assigned_path);
            std::cout<<"\n";
        }

        //std::cout<<"#Nodes in assigned path = "<<result_path->nodes.size()<<" "<<result_path->links.size()<<std::endl;
        //F->assigned_path = result_path;

        std::cout<<"\tDelay along assigned path using RTA= "<<format_time(F->getPathDelay(F->assigned_path))<<" vs. "<<format_time(F->relative_deadline)<<std::endl;
        std::cout<<"\tDelay along assigned path using DCT= "<<format_time(DM->getPathDelay(*(F), *(F->assigned_path)))<<" vs. "<<format_time(F->relative_deadline)<<std::endl;
        std::cout<<"\tDelay along assigned path using Beta = "<<format_time(beta[F->destination]-F->perNodeDelay(F->destination))<<" vs. "<<format_time(F->relative_deadline)<<std::endl;
    }

    //std::cout << "ASHISH: "<<F->flow_name <<" "<<count_useful<<std::endl;
    usefulEdgesCtMap[F->flow_name].push_back(count_useful);
    //std::cout <<"Latency Scale = "<<scale<<std::endl;
    if((EMPTY_NETWORK))
        F->actual_delay = F->getPathDelay(F->assigned_path);
    else
        F->actual_delay = beta[F->destination];

    // Check if the actual delay is higher or lower than the deadline
    if(F->actual_delay <= F->relative_deadline)
    //if(F->getPathDelay(F->assigned_path) <= F->relative_deadline)
        return true;
    else
        return false;

}

bool AdaptiveRouting::semi_adaptive_realtime_routing_rta(RTNetView* rt_nv, bool EMPTY_NETWORK, bool IS_APERIODIC=false){
    // Cost metrics from the graph
    DelayModel *DM = new DelayModel(rt_nv);
    PriorityQueueCustom_Beta PQB;
    Node* u;
    Node* v;

    uint count_useful = 0;

    Node* lowest_cost_link;
    Node* cur_node;

    std::unordered_map<Node*, bool, CustomNodePHash> visited;

    Path* result_path = new Path();
    Path* useful_path = new Path();
    Path* possibly_useful_path = new Path();
    Path* temp_path = new Path();

    Path* P_min_rta;
    Path* P_min_frc;

    Flow* F;
    AperiodicFlow* AF;
    double scale;

    if (IS_APERIODIC)
        AF = rt_nv->cur_aperiodic_flow;
    else
        F = rt_nv->cur_periodic_flow;
    
    // Record the shortest and longest delays from every node to the destination across all the paths
    if (DBG_ADAP_ROU){
        //std::cout<<"\n\n\t\t========================================================"<<std::endl;
        std::cout<<"\n\n"<<std::string(240, '=')<<std::endl;
        std::cout<<"\t\t"<<"***********"<<"EXPECTED DELAY PREPARATION "<<rt_netview->cur_periodic_flow->flow_name<<"************"<<std::endl;
        std::cout<<std::string(240, '=')<<std::endl;
        //std::cout<<"\t\t========================================================"<<std::endl;

        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        std::cout<<"\t\t"<<std::setw(20)<<std::left<<"Path Length"
            <<std::setw(20)<<std::left<<"Exp. Delay (RTA)"
            <<std::setw(20)<<std::left<<"Exp. Delay (FRC)"
            <<std::setw(30)<<std::left<<"Possible Min Path Delay(RTA)"<<"\t"
            <<std::setw(30)<<std::left<<"Possible Min Path Delay(FRC)"<<std::endl;
        //std::cout<<"\t\tFlow"<<"\t\t"<<"Source"<<"\t\t"<<"Destination"<<"\t"<<"Best Delay"<<"\t"<<"Worst Delay"<<std::endl;
        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        //std::cout<<"\t\t=================================================================================="<<std::endl;
    }
    double rta_val, frc_val;


    for (auto P: rt_nv->all_possible_paths){
        if (IS_APERIODIC){
            rta_val = AF->getPathDelay(P);
            frc_val = DM->getPathDelay(*AF,*P);

        }
        else{
            rta_val = F->getPathDelay(P);
            frc_val = DM->getPathDelay(*F,*P);
        }
        if (rta_val <= F->expected_min_delay_rta){
                F->expected_min_delay_rta = rta_val;
                P_min_rta = P;
        }
        if (frc_val <= F->expected_min_delay_frc){
                F->expected_min_delay_frc = frc_val;
                P_min_frc = P;
        }

        if (DBG_ADAP_ROU){
            std::cout<<"\t\t"<<std::setw(20)<<std::left<<(P->links).size()
                <<std::setw(20)<<std::left<<format_time(rta_val)
                <<std::setw(20)<<std::left<<format_time(frc_val);
            //std::cout<<std::setw(20);
            std::cout<<std::left;
            rt_nv->printPath(P_min_rta);
            std::cout<<"\t\t"<<std::right;
            rt_nv->printPath(P_min_frc);
            std::cout<<std::endl;
        }
    }

    if (DBG_ADAP_ROU){
        std::cout<<"\n";
        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        //std::cout<<"\n\t\t========================================================"<<std::endl;
        std::cout<<"\t\t***********END OF DELAY PREPARATION "<<rt_netview->cur_periodic_flow->flow_name<<"************"<<std::endl;
        std::cout<<"\t\t"<<std::string(240, '=')<<std::endl;
        //std::cout<<"\t\t========================================================"<<std::endl;
    }


    if((EMPTY_NETWORK) || (IS_APERIODIC)){
        if (DBG_ADAP_ROU)
            std::cout<<"\n\tAssigning Path using Dijkstra's Shortest Path (Number of Hops)\n";
        result_path = rt_nv->get_shortest_hoplength_path();

        if (IS_APERIODIC)
            AF->assigned_path = result_path;
        else
            F->assigned_path = result_path;
    }

    else {

        //Add all the vertices to the priority queue
        // for (Node* x: A->netview_nodes){
        //     AdaptiveRoutingLabel* ARx = new AdaptiveRoutingLabel(F, x);
        //     ARx->beta = beta[x];
        //     PQ.push(ARx);
        // }

        for (Node* x: rt_nv->rtnetview_nodes){
            visited[x] = false;
        }


        //Add only the source node to the queue
        cur_node = F->source;
        addNode(cur_node, useful_path);

        /* AdaptiveRoutingLabel* ARL_src = new AdaptiveRoutingLabel(F, cur_node);
        ARL_src->beta = beta[cur_node];
        PQ.push(ARL_src);*/

        if (DBG_ADAP_ROU)
            std::cout<<"\n\tStarting at "<<cur_node->get_name()<<std::endl;

        while(cur_node != F->destination){
            //u = PQ.top()->cur_node;
            //PQ.pop();
            //result_path->nodes.push_back(u);

            //exploring the neighbours here
            for (auto l: cur_node->links){
                //reset the possibly_useful_path to useful_path before processing each link     
                possibly_useful_path = new Path(useful_path->nodes, useful_path->links);

                //Process this link
                if(std::find(rt_nv->rtnetview_links.begin(), rt_nv->rtnetview_links.end(), l) != rt_nv->rtnetview_links.end()){
                    v = (l->node1 == cur_node) ? l->node2 : l->node1;
                    
                    if (DBG_ADAP_ROU)
                        std::cout<<"\n\n\t\tSemi-Adaptive Routing: Discovered Link "<<cur_node->node_name<<"----"<<v->get_name()<<"\n";

                    // Edge-Relaxation -- Check if the edge is useful
                    // if ((beta[x]+Cw(x,y)+alpha[y] <= D) && (beta[x] + Ct(x,y) <= beta[y])){
                    
                    // ISSUE: Double counting F->perNodeDelay(v) in both F->perNodeDelay(v) and alpha[v]?
                    /* if ((beta[cur_node] + F->perNodeDelay(cur_node) + F->perLinkDelay(l) + F->perNodeDelay(v) + alpha[v] <= F->relative_deadline) &&
                    *     (beta[cur_node] + F->perNodeDelay(cur_node) + F->perLinkDelay(l) + F->perNodeDelay(v) <= beta[v])){
                    */

                    // Option 1: Define a useful edge condition based on Delay Composition Theorem
                    // temp_path = new Path(possibly_useful_path->nodes, possibly_useful_path->links);
                    // addNode(v, temp_path);
                    // addLink(l, temp_path);
                    // //scale = generate_random_double_seeded(0.0, 1.0);
                    // scale = 1.0;
                    // double updated_end_to_end_delay = (scale * DM->getPathDelay(*(F), *(temp_path))) + (alpha[v]-rt_nv->node_view[v]);//- rt_nv->node_view[v];
                    // double updated_beta = (scale * DM->getPathDelay(*(F), *(temp_path)));
                    // if (DBG_ADAP_ROU){
                    //     std::cout<<"\tConsidering Path = ";
                    //     temp_path->printPath();
                    //     std::cout<<std::endl;
                    //     std::cout<<__LINE__<<"\t\tUpdated End to End Delay = "<<format_time(updated_end_to_end_delay)<<"\t"
                    //         <<"Updated Beta = "<<format_time(updated_beta)<<"\n";
                    // }
                    // if((updated_end_to_end_delay <= F->relative_deadline) && (updated_beta <= beta[v]) && (!visited[v])){
                        
                    //Option 2: Define a useful edge condition based on the expression from Adaptive Routing paper
                    
                    // if ((beta[cur_node] + rt_nv->link_view[l] + alpha[v] <= F->relative_deadline) &&
                    //     (beta[cur_node] + rt_nv->link_view[l] + rt_nv->node_view[v] <= beta[v])){

                    //Option 3: Use RTA-based Delays
                    temp_path = new Path(possibly_useful_path->nodes, possibly_useful_path->links);
                    addNode(v, temp_path);
                    addLink(l, temp_path);
                    double updated_end_to_end_delay = beta[cur_node] + F->perLinkDelay(l) + alpha[v];
                    double updated_beta = beta[cur_node] + F->perLinkDelay(l) + F->perNodeDelay(v);
                    if (DBG_ADAP_ROU){
                        std::cout<<"\t\t\tAdaptive Routing Condition Check: "<<"\n\t\t\t 1. beta["<<cur_node->node_name<<"]="<<format_time(beta[cur_node])<<", "<<"F->perLinkDelay("<<l->link_name<<")="<<format_time(F->perLinkDelay(l))<<", ";
                        std::cout<<"alpha["<<v->get_name()<<"]="<<format_time(alpha[v])<<", "<<"Relative Deadline = "<<format_time(F->relative_deadline)<<std::endl;
                        std::cout<<"\t\t\t 2. beta["<<cur_node->get_name()<<"] + F->perLinkDelay("<<l->link_name<<") + F->perNodeDelay("<<v->node_name<<") = "<<format_time(beta[cur_node] + F->perLinkDelay(l) + F->perNodeDelay(v))<<" Vs."<<"beta[v] = "<<format_time(beta[v])<<std::endl;
                        std::cout<<"\t\t\t 3. visited["<<v->get_name()<<"] = "<<visited[v]<<std::endl;
                    }
                    if ((updated_end_to_end_delay <= F->relative_deadline) && (updated_beta <= beta[v]) && (!visited[v])){               

                        // addNode(v, possibly_useful_path);
                        // addLink(l, possibly_useful_path);
                        count_useful = count_useful + 1;
                        
                        // Option 1:
                        //beta[v] = updated_beta;

                        //Option 2:
                        //beta[v] = beta[cur_node] + rt_nv->node_view[v] + F->perLinkDelay(l);
                        //Option 3: 
                        beta[v] = updated_beta;

                        
                        if (DBG_ADAP_ROU)
                            std::cout<<"\t\t"<<cur_node->node_name<<"----"<<v->get_name()<<" "<<"Useful Edge with RTA DIST(.) = "<<format_time(updated_beta)<<"\tDCT DIST(.) = "<<format_time(DM->getPathDelay(*(F), *(temp_path)))<<std::endl;
                        
                        // beta[v] = CM->getPathDelay(*(F), *(possibly_useful_path));
                        // std::cout<<"Useful Edge with DIST(.) = "<<beta[v]<<std::endl;
                        // result_path->nodes.push_back(v);
                        // result_path->links.push_back(l);

                        AdaptiveRoutingLabel* ARv = new AdaptiveRoutingLabel(F, v);
                        ARv->beta = beta[v];
                        ARv->T = temp_path; //possibly_useful_path;
                        PQB.push(ARv);
                    }
                    else{
                        if (DBG_ADAP_ROU){
                            //std::cout<<"\t\t"<<cur_node->node_name<<"----"<<v->get_name()<<" "<<"Not Useful Edge as it would give end-to-end delay = "<<format_time(beta[cur_node] + F->perLinkDelay(l) + alpha[v])<<std::endl;
                            std::cout<<"\t\t"<<cur_node->node_name<<"----"<<v->get_name()<<" "<<"Not Useful Edge as it would give end-to-end delay = "<<format_time(updated_end_to_end_delay)<<std::endl;
                        }
                    }
                }//done with one link
            
            }//done with all the links on cur_node

            if (DBG_ADAP_ROU)
                std::cout<<"\n\tDone with all the links of "<<cur_node->get_name()<<std::endl;
            
            visited[cur_node] = true;

            if (PQB.empty()){
                if (DBG_ADAP_ROU)
                    std::cout<<"\t\tNo useful edges from "<<cur_node->get_name()<<"-- No Path, going back"<<std::endl;
                // Retrace back to the prev cur_node
                if(useful_path->nodes.size() >= 2){
                    useful_path->nodes.pop_back();
                    useful_path->links.pop_back();
                    cur_node = useful_path->nodes.back();
                }
                else
                    return false;
            }
            //Finding lowest cost path
            else{
                AdaptiveRoutingLabel* AR_lowest_cost = PQB.top();
                if (!AR_lowest_cost){
                    if (DBG_ADAP_ROU)
                        std::cout<<"\t\tAR_lowest_cost not found\n";
                }
                else{
                    if (DBG_ADAP_ROU){
                        std::cout<<"\t\t=============================================="<<std::endl;
                        std::cout<<"\t\tPicked Link "<<cur_node->node_name<<"----"<<AR_lowest_cost->cur_node->get_name()<<"\n";
                    }
                    cur_node = AR_lowest_cost->cur_node;
                    useful_path = AR_lowest_cost->T;
                    if (DBG_ADAP_ROU){
                        std::cout<<"\t\tLowest Cost Next Node : "<<cur_node->get_name()<<"\t";
                        rt_netview->printPath(useful_path);
                        std::cout<<"\n";
                        std::cout<<"\t\t=============================================="<<std::endl;
                    }
                    PQB.pop();
                }
                //Clean up other links and nodes
                //i.e., until PQ is empty
                while(!PQB.empty())
                    PQB.pop();
            }
            if (DBG_ADAP_ROU)
                print_routing_table();
        } // goto the next cur_node and loop until the destination is reached 

        result_path = useful_path;
        F->assigned_path = result_path;
    } // Path Assignment complete

    if (DBG_ADAP_ROU){
        std::cout<<"\n\tSemi-Adaptive Routing Path is : ";
        if (IS_APERIODIC){
            rt_nv->printPath(AF->assigned_path);
            std::cout<<"\n";
        }
        else{
            rt_nv->printPath(F->assigned_path);
            std::cout<<"\n";
        }

        //std::cout<<"#Nodes in assigned path = "<<result_path->nodes.size()<<" "<<result_path->links.size()<<std::endl;
        //F->assigned_path = result_path;

        std::cout<<"\tDelay along assigned path using RTA= "<<format_time(F->getPathDelay(F->assigned_path))<<" vs. "<<format_time(F->relative_deadline)<<std::endl;
        std::cout<<"\tDelay along assigned path using DCT= "<<format_time(DM->getPathDelay(*(F), *(F->assigned_path)))<<" vs. "<<format_time(F->relative_deadline)<<std::endl;
        std::cout<<"\tDelay along assigned path using Beta = "<<format_time(beta[F->destination]-F->perNodeDelay(F->destination))<<" vs. "<<format_time(F->relative_deadline)<<std::endl;
    }

    //std::cout << "ASHISH: "<<F->flow_name <<" "<<count_useful<<std::endl;
    usefulEdgesCtMap[F->flow_name].push_back(count_useful);
    //std::cout <<"Latency Scale = "<<scale<<std::endl;
    if((EMPTY_NETWORK))
        F->actual_delay = F->getPathDelay(F->assigned_path);
    else
        F->actual_delay = beta[F->destination]-F->perNodeDelay(F->destination);

    // Check if the actual delay is higher or lower than the deadline
    if(F->actual_delay <= F->relative_deadline)
    //if(F->getPathDelay(F->assigned_path) <= F->relative_deadline)
        return true;
    else
        return false;
}

bool AdaptiveRouting::semi_adaptive_realtime_routing_nc(RTNetView* rt_nv, bool EMPTY_NETWORK, bool IS_APERIODIC=false){
}


void AdaptiveRouting::fully_adaptive_realtime_routing(RTNetView* rt_nv, bool EMPTY_NETWORK, bool IS_APERIODIC=false){

    //DBG_PRINT
    // Cost metrics from the graph
    DelayModel *DM = new DelayModel(rt_nv);
    PriorityQueueCustom_Beta PQ;

    Path* init_path = new Path();
    Path* starting_path = new Path();
    
    Path* discovered_path = new Path();
    Path* possible_path = new Path();
    Path* best_possible_path = new Path();
    std::unordered_map<Node*, bool, CustomNodePHash> visited;

    double min_pipeline_delay;

    Flow* F;
    AperiodicFlow* AF;

    if (IS_APERIODIC)
        AF = rt_nv->cur_aperiodic_flow;
    else
        F = rt_nv->cur_periodic_flow;

    // //Record the shortest and longest delays from every node to the destination
    // //across all the paths
    std::cout<<"\t\t==================STAGE 3==================="<<std::endl;
    std::cout<<"\t\t****************DELAY PREPARATION*************"<<std::endl;
    std::cout<<"\t\t=============================================="<<std::endl;

    std::cout<<"\t\t=================================================================================="<<std::endl;
    std::cout<<"\t\tDelay(RTA)"<<"\t"<<"Delay(FRC)"<<"\t"<<"Possible Path"<<std::endl;
    //std::cout<<"\t\tFlow"<<"\t\t"<<"Source"<<"\t\t"<<"Destination"<<"\t"<<"Best Delay"<<"\t"<<"Worst Delay"<<std::endl;
    std::cout<<"\t\t=================================================================================="<<std::endl;

    double rta_val, frc_val;

    for (auto P: rt_nv->all_possible_paths){
        if (IS_APERIODIC){
            rta_val = AF->getPathDelay(P);
            frc_val = DM->getPathDelay(*AF,*P);
        }
        else{
            rta_val = F->getPathDelay(P);
            frc_val = DM->getPathDelay(*F,*P);
        }
        std::cout<<"\t\t"<<rta_val<<"\t"<<frc_val<<"\t";
        rt_nv->printPath(P);
        std::cout<<"\n";
    }

    std::cout<<"\t\t=============================================="<<std::endl;
    std::cout<<"\t\t*************END OF DELAY PREPARATION**********"<<std::endl;
    std::cout<<"\t\t=============================================="<<std::endl;


    std::cout<<"\t\t\t==================STAGE 4==================="<<std::endl;
    std::cout<<"\t\t\t****************ADAPTIVE ROUTING*************"<<std::endl;
    std::cout<<"\t\t\t=============================================="<<std::endl;

    // Initially, when the network is empty, the highest priority 
    // flow follows Djikstra's Shortest Path
       
    if((EMPTY_NETWORK) || (IS_APERIODIC)){
        if(IS_APERIODIC)
            AF->assigned_path = rt_nv->get_shortest_hoplength_path();
        else
            F->assigned_path = rt_nv->get_shortest_hoplength_path();
    }
    else{
        Node *x, *y, *z;
        Path *pk;
        Link* lk; 

        x = F->source;
        addNode(x, init_path);
        starting_path = new Path (init_path->nodes, init_path->links);

        if (DBG_ADAP_ROU)
            std::cout<<"\t\t\t\tStarting from "<<x->node_name<<std::endl;

        while (x != F->destination && !visited[x] && std::find(rt_nv->rtnetview_nodes.begin(), rt_nv->rtnetview_nodes.end(), x) != rt_nv->rtnetview_nodes.end()){

            if (DBG_ADAP_ROU)
                std::cout<<"\t\t\t\tGoing through neighbours of "<<x->node_name<<std::endl;

            for (auto l : x->links){

                if(std::find(rt_nv->rtnetview_links.begin(), rt_nv->rtnetview_links.end(), l)!= rt_nv->rtnetview_links.end()){
                    
                    y = (x == l->node1)?l->node2:l->node1;
                    std::cout<<"\n\nFully Adaptive Discove Link "<<x->node_name<<"----"<<y->get_name()<<std::endl;
                    possible_path = new Path(starting_path->nodes, starting_path->links);

                    if (!visited[y]){
                        addNode(y, possible_path);
                        addLink(l, possible_path);

                        //T->printPath(possible_path);
                        double pipeline_delay = DM->getPathDelay(*F, *possible_path);
                        double rta_delay = F->getPathDelay(possible_path);

                        //TODO: Fix this
                        //if ((beta[x]+Cw(x,y)+alpha[y] <= D) && (beta[x] + Ct(x,y) <= beta[y])){
                        if ((beta[x] + rta_delay + alpha[y] <= F->relative_deadline) && (beta[x] + pipeline_delay <= beta[y])){
                            beta[y] = beta[x] + pipeline_delay;
                            T[y] = possible_path;
                        }

                        if (DBG_ADAP_ROU){
                            std::cout<<"\t\t\t\tDelay Measurement = "<<alpha[y]<<" along ";
                        possible_path->printPath();
                        std::cout<<"\n";
                        }
                        AdaptiveRoutingLabel* X = new AdaptiveRoutingLabel(F,y);

                        PQ.push(X);
                        visited[y] = true;
                    }
                    //Reset possible_path
                    possible_path = new Path(discovered_path->nodes, discovered_path->links);

                }
            }

            visited[x] = true;

            x = PQ.top()->cur_node;
            //DBG_PRINT
            if (DBG_ADAP_ROU){
                std::cout<<"\t\t\t\t****Finished going through neighbours****"<<std::endl;
                std::cout<<"\t\t\t\tNext Lowest delay node is  "<<x->node_name<<" "<<std::endl<<std::endl;
            }

            pk = PQ.top()->W;
            //DBG_PRINT
            std::cout<<"No of links in the path = "<<pk->links.size()<<std::endl;
            lk = pk->links[pk->links.size()-1];
            //DBG_PRINT
            if (DBG_ADAP_ROU)
                std::cout<<"\t\t\t\tPicked ("<<lk->link_name<<","<<x->node_name<<") "<<(x)<<" going towards"<<F->source<<std::endl;
            
            //Add the node and link to discovered_path
            addNode(x, discovered_path);
            addLink(lk, discovered_path);

            //Clean up the other links
            while(!PQ.empty()){
                z = PQ.top()->cur_node;
                visited[z] = false;
                PQ.pop();
            }

            //DBG_PRINT
        }

        F->assigned_path = discovered_path;     
        std::cout<<"\t\t\t\tBest Path is :";
        rt_nv->printPath(F->assigned_path);
        std::cout<<"\n";

    }

    std::cout<<"\t\t\t=============================================="<<std::endl;
    std::cout<<"\t\t\t*************END ADAPTIVE ROUTING**********"<<std::endl;
    std::cout<<"\t\t\t=============================================="<<std::endl;

}