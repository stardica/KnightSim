
#ifndef SWITCH_H_
#define SWITCH_H_

#include <desim.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define NUMPACKETS 4
#define NUMPORTS 6
#define MAXQUEUEDEPTH 4
#define SWITCHLATENCY 2

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
	invalid_queue = 0,
	north_queue,
	east_queue,
	south_queue,
	west_queue,
	front_queue,
	back_queue,
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
	int switch_id; //simulation id
	int switch_node_number[3]; //where in topography
	int num_ports;
	int latency;
	int bus_width;
	enum arbitrate arb_style;

	enum port_name current_queue;

	//crossbar
	crossbar_t *crossbar;

	enum switch_crossbar_lane_map next_crossbar_lane;
	enum switch_io_lane_map north_next_io_lane;
	enum switch_io_lane_map east_next_io_lane;
	enum switch_io_lane_map south_next_io_lane;
	enum switch_io_lane_map west_next_io_lane;
	enum switch_io_lane_map front_next_io_lane;
	enum switch_io_lane_map back_next_io_lane;

	//for switches with 6 ports
	list **rx_request_queues;
	list **rx_reply_queues;
	list **tx_request_queues;
	list **tx_reply_queues;

	//io ctrl
	eventcount *switches_north_io_ec;
	eventcount *switches_east_io_ec;
	eventcount *switches_south_io_ec;
	eventcount *switches_west_io_ec;
	eventcount *switches_font_io_ec;
	eventcount *switches_back_io_ec;

};

struct packet_t{

	enum type_name type;
	int size;
	enum port_name dest;
};


typedef struct crossbar_t crossbar;
typedef struct packet_t packet;


struct switch_t *__switch;

eventcount *switch_ec;
eventcount *switch_io_ec;

/*int switch_id;
int producer_pid;
int switch_pid;
int switch_io_pid;*/

void producer_init(void);
void switch_init(void);

void producer_ctrl(void);
void switch_ctrl(void);
void switch_io_ctrl(void);

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
packet *network_packet_create(int type, int dest);
void network_packet_destroy(packet *net_packet);


#endif /*CACHE_H_*/
