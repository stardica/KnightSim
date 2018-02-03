
#ifndef SWITCH_H_
#define SWITCH_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "KnightSim/knightsim.h"

#define NUMSWITCHES 6

#define NUMPACKETS 10
#define NUMPORTS 6

#define MAXQUEUEDEPTH 16
#define SWITCHLATENCY 2

#define BUSWIDTH 8

#define P_TIME (etime->count >> 1)
#define P_PAUSE(p_delay) pause((p_delay)<<1)

#define ENTER_SUB_CLOCK if (!(etime->count & 0x1)) pause(1);
#define EXIT_SUB_CLOCK if (etime->count & 0x1) pause(1);

enum arbitrate{
	round_robin = 0,
	prioity
};

enum switch_crossbar_lane_map{
	crossbar_invalid_lane = 0,
	crossbar_request,
	crossbar_reply,
	crossbar_num_lanes
};

enum switch_io_lane_map{
	io_invalid_lane = 0,
	io_request,
	io_reply,
	io_num_lanes
};

enum type_name{
	request = 0,
	reply,
	type_num
};

enum port_name{
	north_queue = 0,
	east_queue,
	south_queue,
	west_queue,
	front_queue,
	back_queue,
	invalid_queue,
	port_num
};


struct crossbar_t{

	int num_ports;
	int num_links;
	enum port_name current_port;
	enum port_name *out_port_linked_queues;
};


struct switch_t{

	char *name; //simulation name
	int id; //simulation id
	int node_number; //where in topography
	int num_ports;
	int latency;
	int bus_width;
	enum arbitrate arb_style;
	enum port_name current_queue;

	//crossbar
	struct crossbar_t *crossbar;
	enum switch_crossbar_lane_map next_crossbar_lane;
	enum switch_io_lane_map north_next_io_lane;
	enum switch_io_lane_map east_next_io_lane;
	enum switch_io_lane_map south_next_io_lane;
	enum switch_io_lane_map west_next_io_lane;
	enum switch_io_lane_map front_next_io_lane;
	enum switch_io_lane_map back_next_io_lane;

	list **rx_request_queues;
	list **rx_reply_queues;
	list **tx_request_queues;
	list **tx_reply_queues;

	//topology stuff
	int next_east;
	int next_west;
	list *next_east_rx_request_queue;
	list *next_east_rx_reply_queue;
	list *next_west_rx_request_queue;
	list *next_west_rx_reply_queue;

	//I/O ctrl
	eventcount **switch_io_ec;

	pthread_mutex_t mutex;
};

struct packet_t{
	long long id;
	enum type_name type;
	int size;
	int dest[2];
	int direction;
};

typedef struct crossbar_t crossbar;
typedef struct packet_t packet;

void switch_init(void);
void switch_create(void);
void switch_create_tasks(void);
void switch_create_io_tasks(void);

void switch_producer_init(void);
void switch_producer_ctrl(void);

void switch_ctrl(void);
void switch_io_ctrl(void);

list *switch_io_get_next_rx_queue(struct switch_t *switches, enum port_name port, enum switch_io_lane_map current_io_lane);
packet *switch_io_ctrl_get_packet(struct switch_t *switches, enum port_name port, enum switch_io_lane_map current_io_lane);
list *switch_io_ctrl_get_tx_queue(struct switch_t *switches, enum port_name port, enum switch_io_lane_map current_io_lane);
enum switch_io_lane_map get_next_io_lane_rb(enum switch_io_lane_map current_io_lane);
enum port_name get_next_queue_reverse(enum port_name queue);

crossbar *switch_crossbar_create(struct switch_t *__switch);
enum port_name crossbar_get_port_link_status(struct switch_t *__switch);
list *switch_get_rx_queue(struct switch_t *__switch, enum port_name queue);
list *switch_get_tx_queue(struct switch_t *__switch, enum port_name queue);
enum switch_crossbar_lane_map switch_get_next_crossbar_lane(enum switch_crossbar_lane_map current_crossbar_lane);
void switch_crossbar_link(struct switch_t *__switch);
void switch_crossbar_clear_state(struct switch_t *__switch);
packet *switch_get_rx_packet(struct switch_t *__switch);
enum port_name switch_get_packet_route(struct switch_t *__switch, packet *net_packet);
void switch_set_link(struct switch_t *__switch, enum port_name tx_port);
enum port_name get_next_queue_rb(enum port_name queue);
packet *network_packet_create(int type, int *dest);
void network_packet_destroy(packet *net_packet);


#endif /*CACHE_H_*/
