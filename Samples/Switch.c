#include <desim.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#define NUMPACKETS 50
#define NUMPORTS 6
#define MAXQUEUEDEPTH 4

typedef struct crossbar_t crossbar;
typedef struct packet_t packet;

enum arbitrate{

	round_robin = 0,
	prioity

};

enum switch_crossbar_lane_map{

	crossbar_invalid_lane,
	crossbar_request,
	crossbar_reply,
	crossbar_num_lanes
};

enum switch_io_lane_map{

	io_invalid_lane,
	io_request,
	io_reply,
	io_num_lanes
};

enum type_name
{
	request = 0,
	reply,
	type_num
};

enum port_name
{
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

	enum port_name current_port;

	//in queues
	enum port_name north_in_out_linked_queue;
	enum port_name east_in_out_linked_queue;
	enum port_name south_in_out_linked_queue;
	enum port_name west_in_out_linked_queue;
	enum port_name front_in_out_linked_queue;
	enum port_name back_in_out_linked_queue;
};




struct switch_t{

	char *name; //simulation name
	int switch_id; //simulation id
	int switch_node_number[3]; //where in topography
	int num_ports;
	int latency;
	int bus_width;
	enum arbitrate arb_style;

	enum port_name queue;

	//crossbar
	crossbar *crossbar;
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


eventcount *switch_ec;

struct switch_t *__switch;

int switch_id = 0;
int producer_pid = 0;


void producer_init(void);
void producer_ctrl(void);
void switch_init(void);
void switch_ctrl(void);
packet *network_packet_create(int type, int dest);
crossbar *switch_crossbar_create(void);

int main(void){

	//user must initialize DESim
	desim_init();

	producer_init();

	switch_init();

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/
	printf("Simulate %d interactions\n", NUMPACKETS);
	simulate();
	printf("End simulation\n");

	return 1;
}



void producer_ctrl(void){

	int my_pid = producer_pid;
	enum port_name port = producer_pid; //determine who I am 0-5
	producer_pid++;
	int i = 0;

	int type = 0;
	int dest = 0;

	packet *packet_ptr = NULL;

	printf("producer_ctrl %d init\n", my_pid);
	while(i < NUMPACKETS)
	{
		//create packet, keep trying to place into swith rx queue
		type = rand() % 2;
		dest = rand() % NUMPORTS;
		packet_ptr =  network_packet_create(type, dest);

		//requests
		if(packet_ptr->type == request)
		{
			while(desim_list_count(__switch->rx_request_queues[port]) >= MAXQUEUEDEPTH)
			{
				printf("producer_ctrl %d: stalling rx request queue size %d cycle %llu\n", my_pid, desim_list_count(__switch->rx_request_queues[port]), CYCLE);
				pause(1);
			}

			//success clear the packet pointer
			desim_list_enqueue(__switch->rx_request_queues[port], packet_ptr);
		}
		else if(packet_ptr->type == reply)
		{
			while(desim_list_count(__switch->rx_reply_queues[port]) >= MAXQUEUEDEPTH)
			{
				printf("producer_ctrl %d: stalling rx reply queue size %d cycle %llu\n", my_pid, desim_list_count(__switch->rx_reply_queues[port]), CYCLE);
				pause(1);
			}

			//success clear the packet pointer
			desim_list_enqueue(__switch->rx_reply_queues[port], packet_ptr);
		}

		/*printf("producer_ctrl %d: success queue size %d cycle %llu\n",
		my_pid, desim_list_count(__switch->rx_request_queues[port]), CYCLE);*/

		pause(packet_ptr->size / 8); //transfer time
		advance(switch_ec); //wait for entire messages to transfer before signaling the switch
		packet_ptr = NULL;
		i++;
	}

	/*This context finished its work, destroy*/
	context_terminate();

	return;
}

packet *network_packet_create(int type, int dest){

	packet *new_packet = NULL;

	new_packet = (void *) calloc((1), sizeof(packet));
	assert(new_packet);

	//if 1 make packet a request packet
	new_packet->type = type;
	new_packet->dest = dest;

	if(type == request)
		new_packet->size = 8;
	else
		new_packet->size = 64;

	return new_packet;
}

void producer_init(void){

	int i = 0;
	char buff[100];

	/*producers*/
	for(i = 0; i < NUMPORTS; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "producer_%d", i);
		context_create(producer_ctrl, 32768, strdup(buff));
	}

	srand(time(NULL));

	return;
}

void switch_init(void){

	char buff[100];
	int i = 0;


	//create the user defined eventcounts
	memset(buff,'\0' , 100);
	snprintf(buff, 100, "switch_ec");
	switch_ec = eventcount_create(strdup(buff));


	//create the switch
	__switch = (void *) calloc((1), sizeof(struct switch_t));

	memset (buff,'\0' , 100);
	snprintf(buff, 100, "switch[%d]", switch_id);
	__switch->name = strdup(buff);

	__switch->switch_id = switch_id++;

	__switch->num_ports = NUMPORTS;

	//configure cross bar
	__switch->crossbar = switch_crossbar_create();

	//new switch virtual queues
	__switch->rx_request_queues = (void *) calloc((__switch->num_ports), sizeof(list));
	__switch->rx_reply_queues = (void *) calloc((__switch->num_ports), sizeof(list));
	__switch->tx_request_queues = (void *) calloc((__switch->num_ports), sizeof(list));
	__switch->tx_reply_queues = (void *) calloc((__switch->num_ports), sizeof(list));


	for(i = 0; i < __switch->num_ports; i++)
	{
		__switch->rx_request_queues[i] = desim_list_create(MAXQUEUEDEPTH);
		__switch->rx_reply_queues[i] = desim_list_create(MAXQUEUEDEPTH);
		__switch->tx_request_queues[i] = desim_list_create(MAXQUEUEDEPTH);
		__switch->tx_reply_queues[i] = desim_list_create(MAXQUEUEDEPTH);
	}

	return;
}

crossbar *switch_crossbar_create(void){

	crossbar *crossbar_ptr = NULL;

	crossbar_ptr = (void *) malloc(sizeof(crossbar));

	//set up the cross bar variables
	crossbar_ptr->num_ports = 6;

	crossbar_ptr->north_in_out_linked_queue = invalid_queue;
	crossbar_ptr->east_in_out_linked_queue = invalid_queue;
	crossbar_ptr->south_in_out_linked_queue = invalid_queue;
	crossbar_ptr->west_in_out_linked_queue = invalid_queue;
	crossbar_ptr->front_in_out_linked_queue = invalid_queue;
	crossbar_ptr->back_in_out_linked_queue = invalid_queue;

	return crossbar_ptr;
}
