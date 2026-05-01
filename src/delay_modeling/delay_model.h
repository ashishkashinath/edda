#ifndef cost_metrics_h
#define cost_metrics_h

#include <flow_factory.h>
#include <topo_factory.h>
#include <netdesign_factory.h>
#include <util.h>

/*
* DelayModel class contains methods to calcuate delays on a RTNetView (via Delay Composition Analysis)
* The Delay Model used is based on the paper, "End-to-End Delay Analysis of Distributed Systems with Cycles in the Task Graph"
* published in ECRTS 2009, by Jayachandran and Abdelzaher
*/
class DelayModel{
        
        RTNetView* rt_netview;
    
    public:
        DelayModel(RTNetView *T);
        bool isPlannedPeriodic(Flow* F);
        bool isPlannedAperiodic(AperiodicFlow* F);
        std::vector<Flow*>& getHigherPriorityFlows(const Flow& F, std::vector<Flow*>& result);
        std::vector<Flow*>& getHigherPriorityPlannedFlows(const Flow& F, std::vector<Flow*>& result);
        Flow* getFlowById(uint flow_id, Flow* result);
        
        void getCommonSegments(std::vector<Node*>&, Path&, std::vector<std::vector<Node*>>&, 
            std::vector<std::vector<Node*>>&, std::vector<std::vector<Node*>>&);
        double getMaxSingleSwitchProcessingTimeOverCommonSegment(uint flow_id, std::vector<Node*>& common_segment);        
        void getFoldsbyFlowId(uint flow_id, std::vector<std::vector<Node*>>& folds);
        std::vector<Flow*>& getFlowsSharingSwitch(Node* N, std::vector<Flow*>& flows_sharing_switch);
        double getPathDelay(Flow& F, Path& P);
        void printPathSet(std::vector<std::vector<Node*>>& x);
        void printPath(std::vector<Node*>& x);

};

#endif