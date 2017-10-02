#include "Switch.h"


int switch_id = 0;
int producer_pid = 0;
int switch_pid = 0;
int switch_io_pid = 0;


int main(void){

	//user must initialize DESim
	desim_init();

	producer_init();

	switch_init();

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/
	printf("Start switch simulation %d network packets\n", NUMPACKETS);


	simulate();


	printf("End simulation\n");

	return 1;
}

void switch_io_ctrl(void){

	//int my_pid = switch_io_pid++;

	count_t step = 1;
	int i = 0;
	int num_packets = 0;
	packet *net_packet = NULL;

	while(1)
	{
		await(switch_io_ec, step);


		//switch has been advanced
		P_PAUSE(1);

		//just an exmple clear one packet out of each queue
		for(i = 0 ; i < __switch->num_ports; i++)
		{
			net_packet = (packet *)desim_list_remove_at(__switch->tx_request_queues[i], 0);

			if(net_packet)
			{
				network_packet_destroy(net_packet);
				num_packets++;
			}

			net_packet = (packet *)desim_list_remove_at(__switch->tx_reply_queues[i], 0);

			if(net_packet)
			{
				network_packet_destroy(net_packet);
				num_packets++;
			}
		}

		step += num_packets;
		num_packets = 0;
	}

	fatal("switch_io_ctrl(): exiting\n");
	return;
}


void switch_ctrl(void){

	int my_pid = switch_pid++;

	count_t step = 1;

	int i = 0;
	packet *net_packet = NULL;
	list *out_queue = NULL;

	//packet *packet_ptr = NULL;
	printf("switch_ctrl %d init\n", my_pid);
	while(1)
	{
		await(switch_ec, step);
		/*printf("ec count %llu step %llu\n", switch_ec->count, step);*/

		//switch has been advanced
		P_PAUSE(__switch->latency);

		/*wait and enter the subclock domain
		to see if anyone else advance the switch this cycle*/
		ENTER_SUB_CLOCK

		/*models a cross bar link as many inputs to outputs
		as possible in a given cycle*/
		switch_crossbar_link(__switch);

		EXIT_SUB_CLOCK

		for(i = 0; i < __switch->crossbar->num_ports; i++)
		{

			if(crossbar_get_port_link_status(__switch) != invalid_queue)
			{
				/*the out queue is linked to an input queue and the output queue isn't full
				move the packet from the input queue to the correct output queue*/

				//careful here, the next line is for the INPUT queue...
				net_packet = (packet *)desim_list_remove_at(switch_get_rx_queue(__switch, crossbar_get_port_link_status(__switch)), 0);
				assert(net_packet);

				//get a pointer to the output queue
				out_queue = switch_get_tx_queue(__switch, __switch->crossbar->current_port);

				desim_list_enqueue(out_queue, net_packet);
				advance(switch_io_ec);

				net_packet = NULL;
			}

			__switch->crossbar->current_port = get_next_queue_rb(__switch->crossbar->current_port);
		}

		step += __switch->crossbar->num_links;

		printf("formed links %d\n", __switch->crossbar->num_links);
		//getchar();

		//clear the current cross bar state
		switch_crossbar_clear_state(__switch);

		//set the next direction and lane to process
		__switch->next_crossbar_lane = switch_get_next_crossbar_lane(__switch->next_crossbar_lane);
		__switch->crossbar->current_port = get_next_queue_rb(__switch->crossbar->current_port);
		__switch->current_queue = get_next_queue_rb(__switch->current_queue);

		//set message_paket null
		net_packet = NULL;

	}

	fatal("switch_ctrl(): outside of while loop\n");

	return;
}

list *switch_get_tx_queue(struct switch_t *__switch, enum port_name queue){

	struct list_t *switch_queue = NULL;


	switch(__switch->next_crossbar_lane)
	{
		case crossbar_request:
			switch_queue = __switch->tx_request_queues[queue - 1];
			break;
		case crossbar_reply:
			switch_queue = __switch->tx_reply_queues[queue - 1];
			break;
		case crossbar_invalid_lane:
		default:
			fatal("switch_get_rx_packet() Invalid port name\n");
			break;
	}

	assert(switch_queue != NULL);
	return switch_queue;
}

list *switch_get_rx_queue(struct switch_t *__switch, enum port_name queue){

	list *switch_queue = NULL;

	switch(__switch->next_crossbar_lane)
	{
		case crossbar_request:
			switch_queue = __switch->rx_request_queues[queue - 1];
			break;
		case crossbar_reply:
			switch_queue = __switch->rx_reply_queues[queue - 1];
			break;
		case crossbar_invalid_lane:
		default:
			fatal("switch_get_rx_packet() Invalid port name\n");
			break;
	}

	assert(switch_queue != NULL);
	return switch_queue;
}



enum port_name crossbar_get_port_link_status(struct switch_t *__switch){

	enum port_name port_status = invalid_queue;

	/*check output link status*/

	port_status = __switch->crossbar->out_port_linked_queues[__switch->crossbar->current_port - 1];

	return port_status;
}

enum switch_crossbar_lane_map switch_get_next_crossbar_lane(enum switch_crossbar_lane_map current_crossbar_lane){

	enum switch_crossbar_lane_map next_lane = crossbar_invalid_lane;

	switch(current_crossbar_lane)
	{
		case crossbar_request:
			next_lane = crossbar_reply;
			break;
		case crossbar_reply:
			next_lane = crossbar_request;
			break;
		case crossbar_invalid_lane:
		default:
			fatal("get_next_io_lane_rb() Invalid port name\n");
			break;
	}

	assert(next_lane != crossbar_invalid_lane);
	return next_lane;
}

void switch_crossbar_link(struct switch_t *__switch){

	packet *net_packet = NULL;

	enum port_name tx_port = invalid_queue;
	int i = 0;

	if(__switch->arb_style == round_robin)
	{
		/*printf("start_2 queue %d\n", switches->queue);*/

		for(i = 0; i < __switch->crossbar->num_ports; i++)
		{
			//check for a waiting packet...
			net_packet = switch_get_rx_packet(__switch);

			//found a new packet, try to link it...
			if(net_packet)
			{
				tx_port = switch_get_packet_route(__switch, net_packet);

				//try to assign the link
				switch_set_link(__switch, tx_port);
			}

			assert(__switch->current_queue >= 1 && __switch->current_queue <= __switch->crossbar->num_ports);

			//advance the queue pointer.
			__switch->current_queue = get_next_queue_rb(__switch->current_queue);
		}
	}
	else
	{
		fatal("switch_crossbar_link(): Invalid ARB set\n");
	}

	/*its possible to not be successful in a given cycle,
	but should have no more than the number of ports on the switch.*/
	assert((__switch->crossbar->num_links >= 0) && (__switch->crossbar->num_links <= __switch->crossbar->num_ports));
	return;
}

crossbar *switch_crossbar_create(struct switch_t *__switch){

	int i = 0;

	crossbar *crossbar_ptr;

	crossbar_ptr = (crossbar *) malloc(sizeof(crossbar));

	//set up the cross bar variables
	crossbar_ptr->num_ports = __switch->num_ports;
	crossbar_ptr->num_links = 0;

	crossbar_ptr->out_port_linked_queues = (enum port_name *) calloc(crossbar_ptr->num_ports, sizeof(enum port_name));

	for(i = 0 ; i < crossbar_ptr->num_ports; i++)
	{
		crossbar_ptr->out_port_linked_queues[i] = invalid_queue;
	}

	return crossbar_ptr;
}

void switch_crossbar_clear_state(struct switch_t *__switch){

	int i = 0;

	//clear the cross bar state
	__switch->crossbar->num_links = 0;

	for(i = 0 ; i < __switch->crossbar->num_ports; i++)
	{
		__switch->crossbar->out_port_linked_queues[i] = invalid_queue;
	}

	return;
}

enum port_name get_next_queue_rb(enum port_name queue){

	enum port_name next_queue = invalid_queue;

	switch(queue)
	{
		case west_queue:
			next_queue = north_queue;
			break;
		case north_queue:
			next_queue = east_queue;
			break;
		case east_queue:
			next_queue = south_queue;
			break;
		case south_queue:
			next_queue = front_queue;
			break;
		case front_queue:
			next_queue = back_queue;
			break;
		case back_queue:
			next_queue = west_queue;
			break;
		case invalid_queue:
		default:
			fatal("get_next_queue() Invalid port name\n");
			break;
	}

	assert(next_queue != invalid_queue);
	return next_queue;
}

void switch_set_link(struct switch_t *__switch, enum port_name tx_port){

	/*we have the in port with switches->current_queue
	 * and the out port with tx_port try to link them...*/

	/*we still a valid input port number switches->current_queue*/

	//destination...
	switch(__switch->next_crossbar_lane)
	{
		case crossbar_request: //if the dest port queue isn't full and it hasn't previously been linked then link it.
			if((desim_list_count(__switch->tx_request_queues[tx_port - 1]) < MAXQUEUEDEPTH) && (__switch->crossbar->out_port_linked_queues[tx_port - 1] == invalid_queue))
			{
					//links input port to output port
					__switch->crossbar->out_port_linked_queues[tx_port - 1] = __switch->current_queue;
					__switch->crossbar->num_links++;
			}
			break;
		case crossbar_reply:
			if((desim_list_count(__switch->tx_reply_queues[tx_port - 1]) < MAXQUEUEDEPTH) && (__switch->crossbar->out_port_linked_queues[tx_port - 1] == invalid_queue))
			{
					__switch->crossbar->out_port_linked_queues[tx_port - 1] = __switch->current_queue;
					__switch->crossbar->num_links++;
			}
			break;
		case crossbar_invalid_lane:
		default:
			fatal("switch_get_rx_packet() Invalid port name\n");
			break;
	}

	return;
}

enum port_name switch_get_packet_route(struct switch_t *__switch, packet *net_packet){

	enum port_name tx_port = invalid_queue;

	//a real switch simulation would model a much more complicated routing algorithm here

	tx_port = (enum port_name)(net_packet->dest + 1);

	assert(tx_port > 0 && tx_port <= __switch->num_ports);
	return tx_port;
}

packet *switch_get_rx_packet(struct switch_t *__switch){

	packet *net_packet = NULL;

	//sanity check
	assert(__switch->current_queue > 0 && __switch->current_queue <= __switch->num_ports);

	switch(__switch->next_crossbar_lane)
	{
		case crossbar_request:
			net_packet = (packet *)desim_list_get(__switch->rx_request_queues[__switch->current_queue - 1], 0);
			break;
		case crossbar_reply:
			net_packet = (packet *)desim_list_get(__switch->rx_reply_queues[__switch->current_queue - 1], 0);
			break;
		case crossbar_invalid_lane:
		default:
			fatal("switch_get_rx_packet() Invalid port name\n");
			break;
	}

	return net_packet;
}

void producer_ctrl(void){

	int my_pid = producer_pid;
	enum port_name port = (enum port_name)producer_pid; //determine who I am 0-5
	producer_pid++;
	int i = 0;

	int type = 0;
	int dest = 0;

	packet *packet_ptr = NULL;

	printf("producer_ctrl %d init\n", my_pid);
	while(i < NUMPACKETS)
	{
		//create packet, keep trying to place into swith rx queue
		type = rand() % 2; //random request or reply packet
		dest = rand() % NUMPORTS; //random destination
		packet_ptr =  network_packet_create(type, dest); //create the packet and give it a size

		//requests
		if(packet_ptr->type == request)
		{
			while(desim_list_count(__switch->rx_request_queues[port]) >= MAXQUEUEDEPTH)
			{
				printf("producer_ctrl %d: stalling rx request queue size %d cycle %llu\n",
						my_pid, desim_list_count(__switch->rx_request_queues[port]), CYCLE);
				P_PAUSE(1);
			}

			//success clear the packet pointer
			desim_list_enqueue(__switch->rx_request_queues[port], packet_ptr);
		}
		else if(packet_ptr->type == reply)
		{
			while(desim_list_count(__switch->rx_reply_queues[port]) >= MAXQUEUEDEPTH)
			{
				printf("producer_ctrl %d: stalling rx reply queue size %d cycle %llu\n",
						my_pid, desim_list_count(__switch->rx_reply_queues[port]), CYCLE);
				P_PAUSE(1);
			}

			//success clear the packet pointer
			desim_list_enqueue(__switch->rx_reply_queues[port], packet_ptr);
		}

		/*printf("producer_ctrl %d: success queue size %d cycle %llu\n",
		my_pid, desim_list_count(__switch->rx_request_queues[port]), CYCLE);*/

		P_PAUSE(packet_ptr->size / 8); //transfer time
		advance(switch_ec); //wait for entire messages to transfer before signaling the switch
		packet_ptr = NULL;
		i++;
	}

	/*This context finished its work, destroy*/
	//printf("producer_ctrl %d exiting\n", my_pid);
	//getchar();
	context_terminate();

	return;
}

void network_packet_destroy(packet *net_packet){

	free(net_packet);
	return;
}

packet *network_packet_create(int type, int dest){

	packet *new_packet = NULL;

	new_packet = (packet *) calloc((1), sizeof(packet));
	assert(new_packet);

	//if 1 make packet a request packet
	new_packet->type = (enum type_name)type;
	new_packet->dest = (enum port_name)dest;

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

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "switch_io_ec");
	switch_io_ec = eventcount_create(strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "switch_ctrl_%d", i);
	context_create(switch_ctrl, 32768, strdup(buff));

	memset(buff,'\0' , 100);
	snprintf(buff, 100, "switch_io_ctrl_%d", i);
	context_create(switch_io_ctrl, 32768, strdup(buff));

	//create the switch
	__switch = (struct switch_t *) calloc((1), sizeof(struct switch_t));

	memset (buff,'\0' , 100);
	snprintf(buff, 100, "switch[%d]", switch_id);
	__switch->name = strdup(buff);

	__switch->switch_id = switch_id++;

	__switch->num_ports = NUMPORTS;

	__switch->latency = SWITCHLATENCY;

	__switch->arb_style = round_robin;

	//create switch virtual queues...
	__switch->rx_request_queues = (list **) calloc((__switch->num_ports), sizeof(list));
	__switch->rx_reply_queues = (list **) calloc((__switch->num_ports), sizeof(list));
	__switch->tx_request_queues = (list **) calloc((__switch->num_ports), sizeof(list));
	__switch->tx_reply_queues = (list **) calloc((__switch->num_ports), sizeof(list));

	//allocate the queues...
	for(i = 0; i < __switch->num_ports; i++)
	{
		__switch->rx_request_queues[i] = desim_list_create(MAXQUEUEDEPTH);
		__switch->rx_reply_queues[i] = desim_list_create(MAXQUEUEDEPTH);
		__switch->tx_request_queues[i] = desim_list_create(MAXQUEUEDEPTH);
		__switch->tx_reply_queues[i] = desim_list_create(MAXQUEUEDEPTH);
	}

	//create the crossbar
	__switch->crossbar = switch_crossbar_create(__switch);

	//init switch and crossbar pointers
	__switch->current_queue = west_queue;
	__switch->next_crossbar_lane = crossbar_request;
	__switch->north_next_io_lane = io_request;
	__switch->east_next_io_lane = io_request;
	__switch->south_next_io_lane = io_request;
	__switch->west_next_io_lane = io_request;
	__switch->crossbar->current_port = west_queue;

	return;
}
