#ifndef eval_parameters_h
#define eval_parameters_h

#include <vector>
#include <debug_config.h>

#define NODE_CAPACITY 1000000

#define RES_DIR ""

// const std::vector<double> pkt_sizes{64.0, 128.0, 256.0, 512.0, 1024.0, 1518.0};
// const std::vector<double> period_sizes{10.0, 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0};
// std::vector<double> pkt_size_bytes_lst = {128, 256, 512, 1024};

const short PRIORITY_MIN = 7;
const short PRIORITY_MAX = 0;
const short STEP_SIZE_PRIORITY = 1;

//propagation + transmission delay ranges (in millisecond)
//((1024*8)/(10 * 1000 * 1000)) * 1000 = 0.8192 millisecond

const float PROP_DELAY_MIN = 0.8192 + 0.000505;  // 505 nanosecond propagation delay, 0.8192 transmission delay
const float PROP_DELAY_MAX = 0.8192 + 0.000505;

/*
* Design Space Exploration Settings
*/
const short int ITER_MAX = 10000;

//Packet Size Design Space Exploration
const double PKT_SIZE_BYTES_MIN = 1024.0;
const double PKT_SIZE_BYTES_MAX = 1024.0;
const double PKT_SIZE_BYTES_INCREMENT = 128.0;

//Flow Bandwidth Design Space Exploration
const double MIN_FLOW_BW = 100E6; //100 Mbps
const double MAX_FLOW_BW = 100E6; //100 Mbps
const double STEP_SIZE_FLOW_BW = 10E6; //10 Mbps

// Period Design Space Exploration
const double MIN_FLOW_PERIOD_SEC = 100.0 / US_PER_SEC; //0.2 ms
const double MAX_FLOW_PERIOD_SEC = 100.0 / US_PER_SEC; //5 ms
const double STEP_SIZE_FLOW_PERIOD_SEC = 200.0 / US_PER_SEC; //0.2 ms

//Deadline Design Space Exploration
const double DEADLINE_MIN_SEC = 100.0 / US_PER_SEC; //0 us
const double DEADLINE_MAX_SEC = 501.0 / US_PER_SEC; //1 ms
const double DEADLINE_INCREMENT_SEC = 0.1 / MS_PER_SEC; //10 us

//Link Capacity Design Space Exploration
const double MIN_LINK_CAP_TYPICAL = 10E9; //1 Gbps
const double MAX_LINK_CAP_TYPICAL = 10E9; //1 Gbps
const double MIN_LINK_CAP_WC = 10E9; //1 Gbps
const double MAX_LINK_CAP_WC = 10E9; // 1 Gbps

//Node Capacity Design Space Exploration
const double MIN_NODE_CAP_BPS = 10E9; // 10 Gbps
const double MAX_NODE_CAP_BPS = 10E9; // 10 Gbps
const double STEP_SIZE_NODE_CAP_BPS = 500E6; //500 Mbps

const int OOS_RATIO = 5; // 1 in 5 flows are out of specification

#endif
