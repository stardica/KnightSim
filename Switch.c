#include "Switch.h"

int producer_pid = 0;
int switch_pid = 0;
int switch_io_pid = 0;
int switch_io_port_pid = 0;
long long packet_id = 0;

int packets_sent = 0;
int packets_received = 0;

struct switch_t **__switch;
eventcount **switch_ec;

int main(void){

	//user must initialize DESim
	desim_init();

	switch_init();

	switch_producer_init();

	/*starts simulation and won't return until simulation
	is complete or all contexts complete*/
	printf("Start switch simulation %d network packets\n", NUMPACKETS * NUMSWITCHES);

	simulate();

	printf("End simulation, packets sent %d packets received %d cycles %llu\n",
			packets_sent, packets_received, P_TIME);

	return 1;
}


packet *switch_io_ctrl_get_packet(struct switch_t *switches, enum port_name port, enum switch_io_lane_map current_io_lane){

	packet *net_packet = NULL;

	switch(current_io_lane)
	{
		case io_request:
			net_packet = (packet *)desim_list_get(switches->tx_request_queues[port], 0);
			break;
		case io_reply:
			net_packet = (packet *)desim_list_get(switches->tx_reply_queues[port], 0);
			break;
		case io_invalid_lane:
		default:
			fatal("get_next_queue() Invalid port name\n");
			break;
	}

	return net_packet;
}

list *switch_io_ctrl_get_tx_queue(struct switch_t *switches, enum port_name port, enum switch_io_lane_map current_io_lane){

	list *tx_queue = NULL;

	switch(current_io_lane)
	{
		case io_request:
			tx_queue = switches->tx_request_queues[port];
			break;
		case io_reply:
			tx_queue = switches->tx_reply_queues[port];
			break;
		case io_invalid_lane:
		default:
			fatal("get_next_queue() Invalid port name\n");
			break;
	}

	return tx_queue;
}


enum switch_io_lane_map get_next_io_lane_rb(enum switch_io_lane_map current_io_lane){

	enum switch_io_lane_map next_lane = io_invalid_lane;

	switch(current_io_lane)
	{
		case io_request:
			next_lane = io_reply;
			break;
		case io_reply:
			next_lane = io_request;
			break;
		case io_invalid_lane:
		default:
			fatal("get_next_io_lane_rb() Invalid port name\n");
			break;
	}


	assert(next_lane != io_invalid_lane);
	return next_lane;
}

list *switch_io_get_next_rx_queue(struct switch_t *switches, enum port_name port, enum switch_io_lane_map current_io_lane){

	list * next_rx_queue = NULL;
	enum port_name next_queue;

	next_queue = get_next_queue_reverse(port);

	switch(next_queue)
	{
		case west_queue:
			switch(current_io_lane)
			{
				case io_request:
					next_rx_queue = switches->next_west_rx_request_queue;
					break;
				case io_reply:
					next_rx_queue = switches->next_west_rx_reply_queue;
					break;
				default:
					fatal("get_next_queue() Invalid port name\n");
					break;
			}
			break;

		case east_queue:
			switch(current_io_lane)
			{
				case io_request:
					next_rx_queue = switches->next_east_rx_request_queue;
					break;
				case io_reply:
					next_rx_queue = switches->next_east_rx_reply_queue;
					break;
				default:
					fatal("get_next_queue() Invalid port name\n");
					break;
			}
			break;

		case north_queue:
		case south_queue:
		case front_queue:
		case back_queue:
		case invalid_queue:
		default:
			fatal("get_next_queue() Invalid port name\n");
			break;
	}

	assert(next_rx_queue);
	return next_rx_queue;
}


void switch_io_ctrl(void){

	int total_ports = NUMPORTS * NUMSWITCHES;

	int switch_pid = switch_io_pid % NUMSWITCHES; //should give 0 - 31
	int port_pid = switch_io_port_pid;

	if(!((total_ports - 1 - switch_io_pid) % NUMSWITCHES))
		switch_io_port_pid++;

	switch_io_pid++;

	//printf("s pid %d p pid %d\n", switch_pid, port_pid);

	assert(switch_pid >= 0 && switch_pid < NUMSWITCHES);
	assert(port_pid >= 0 && port_pid < NUMPORTS);

	count_t step = 1;
	//int i = 0;
	//int num_packets = 0;
	packet *net_packet = NULL;
	int transfer_time = 0;
	enum switch_io_lane_map current_lane = io_request;

	//printf("switch_io_ctrl sw %d port %d init\n", switch_pid, port_pid);

	while(1)
	{
		await(__switch[switch_pid]->switch_io_ec[port_pid], step);

		//switch has been advanced
		P_PAUSE(1);

		net_packet = switch_io_ctrl_get_packet(__switch[switch_pid], (enum port_name)port_pid, current_lane);

		if(!net_packet) //try the next lane...
		{
			current_lane = get_next_io_lane_rb(current_lane);
			continue;
		}

		printf("switch_io_ctrl sw %d port %d advanced packet id %llu\n", switch_pid, port_pid, net_packet->id);

		transfer_time = (net_packet->size/__switch[switch_pid]->bus_width);

		if(transfer_time == 0)
			transfer_time = 1;

		/*north_queue
			south_queue
			front_queue
			back_queue*/

		if(port_pid == east_queue || port_pid == west_queue)
		{
			if(desim_list_count(switch_io_get_next_rx_queue(__switch[switch_pid], (enum port_name)port_pid, current_lane)) >= MAXQUEUEDEPTH)
			{
				P_PAUSE(1);
			}
			else
			{
				step++;

				P_PAUSE(transfer_time);

				net_packet = (packet *)desim_list_remove(switch_io_ctrl_get_tx_queue(__switch[switch_pid], (enum port_name)port_pid, current_lane), net_packet);
				desim_list_enqueue(switch_io_get_next_rx_queue(__switch[switch_pid], (enum port_name)port_pid, current_lane), net_packet);

				if(port_pid == east_queue)
					advance(switch_ec[__switch[switch_pid]->next_west]);
				else
					advance(switch_ec[__switch[switch_pid]->next_east]);
			}
		}
		else
		{
			step++;
			P_PAUSE(transfer_time);
			packets_received++;

			//destroy the packet
			net_packet = (packet *)desim_list_remove(switch_io_ctrl_get_tx_queue(__switch[switch_pid], (enum port_name)port_pid, current_lane), net_packet);

			network_packet_destroy(net_packet);
		}

		current_lane = get_next_io_lane_rb(current_lane);
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
	//printf("switch_ctrl %d init\n", my_pid);

	while(1)
	{
		await(switch_ec[my_pid], step);
		/*printf("ec count %llu step %llu\n", switch_ec->count, step);*/

		/*wait and enter the subclock domain
		to see if anyone else advance the switch this cycle*/
		ENTER_SUB_CLOCK

		/*models a cross bar link as many inputs to outputs
		as possible in a given cycle*/
		switch_crossbar_link(__switch[my_pid]);

		EXIT_SUB_CLOCK

		//charge latency
		P_PAUSE(__switch[my_pid]->latency);

		for(i = 0; i < __switch[my_pid]->crossbar->num_ports; i++)
		{

			if(crossbar_get_port_link_status(__switch[my_pid]) != invalid_queue)
			{
				/*the out queue is linked to an input queue and the output queue isn't full
				move the packet from the input queue to the correct output queue*/

				//careful here, the next line is for the INPUT queue...
				net_packet = (packet *)desim_list_remove_at(switch_get_rx_queue(__switch[my_pid], crossbar_get_port_link_status(__switch[my_pid])), 0);
				assert(net_packet);

				/*enum port_name{
					north_queue = 0,
					east_queue,
					south_queue,
					west_queue,
					front_queue,
					back_queue,
					invalid_queue,
					port_num
				};*/

				printf("switch %d routing packet id %llu dest %d dest port %d cycle %llu\n",
						__switch[my_pid]->id, net_packet->id, net_packet->dest[0], net_packet->dest[1], P_TIME);



				//get a pointer to the output queue
				out_queue = switch_get_tx_queue(__switch[my_pid], __switch[my_pid]->crossbar->current_port);

				desim_list_enqueue(out_queue, net_packet);

				advance(__switch[my_pid]->switch_io_ec[__switch[my_pid]->crossbar->current_port]);
				net_packet = NULL;

			}

			__switch[my_pid]->crossbar->current_port = get_next_queue_rb(__switch[my_pid]->crossbar->current_port);
		}

		step += __switch[my_pid]->crossbar->num_links;

		//printf("formed links %d\n", __switch[my_pid]->crossbar->num_links);
		//getchar();

		//clear the current cross bar state
		switch_crossbar_clear_state(__switch[my_pid]);

		//set the next direction and lane to process
		__switch[my_pid]->next_crossbar_lane = switch_get_next_crossbar_lane(__switch[my_pid]->next_crossbar_lane);
		__switch[my_pid]->crossbar->current_port = get_next_queue_rb(__switch[my_pid]->crossbar->current_port);
		__switch[my_pid]->current_queue = get_next_queue_rb(__switch[my_pid]->current_queue);

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
			switch_queue = __switch->tx_request_queues[queue];
			break;
		case crossbar_reply:
			switch_queue = __switch->tx_reply_queues[queue];
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
			switch_queue = __switch->rx_request_queues[queue];
			break;
		case crossbar_reply:
			switch_queue = __switch->rx_reply_queues[queue];
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

	port_status = __switch->crossbar->out_port_linked_queues[__switch->crossbar->current_port];

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

			assert(__switch->current_queue >= 0 && __switch->current_queue < __switch->crossbar->num_ports);

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

enum port_name get_next_queue_reverse(enum port_name queue){

	enum port_name next_queue = invalid_queue;

	switch(queue)
	{
		case west_queue:
			next_queue = east_queue;
			break;
		case north_queue:
			next_queue = south_queue;
			break;
		case east_queue:
			next_queue = west_queue;
			break;
		case south_queue:
			next_queue = north_queue;
			break;
		case front_queue:
			next_queue = back_queue;
			break;
		case back_queue:
			next_queue = front_queue;
			break;
		case invalid_queue:
		default:
			fatal("get_next_queue() Invalid port name\n");
			break;
	}

	assert(next_queue != invalid_queue);
	return next_queue;
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
			if((desim_list_count(__switch->tx_request_queues[tx_port]) < MAXQUEUEDEPTH) && (__switch->crossbar->out_port_linked_queues[tx_port] == invalid_queue))
			{
					//links input port to output port
					__switch->crossbar->out_port_linked_queues[tx_port] = __switch->current_queue;
					__switch->crossbar->num_links++;

					//printf("switch %d formed link\n", __switch->id);
			}
			break;
		case crossbar_reply:
			if((desim_list_count(__switch->tx_reply_queues[tx_port]) < MAXQUEUEDEPTH) && (__switch->crossbar->out_port_linked_queues[tx_port] == invalid_queue))
			{
					__switch->crossbar->out_port_linked_queues[tx_port] = __switch->current_queue;
					__switch->crossbar->num_links++;

					//printf("switch %d formed link\n", __switch->id);
			}
			break;
		case crossbar_invalid_lane:
		default:
			fatal("switch_get_rx_packet() Invalid port name\n");
			break;
	}

	return;
}

//a real switch simulation would model a much more complicated routing algorithm here
enum port_name switch_get_packet_route(struct switch_t *__switch, packet *net_packet){

	enum port_name tx_port = invalid_queue;

	int x = 0;
	int y = 0;

	//determine if this is the dest switch
	if(__switch->id == net_packet->dest[0])
	{
		assert(net_packet->dest[0] >= 0 && net_packet->dest[0] < NUMSWITCHES);
		assert(net_packet->dest[1] != east_queue && net_packet->dest[1] != west_queue);
		tx_port = (enum port_name)net_packet->dest[1];
	}
	else
	{
		//packet need to go to another switch
		if(net_packet->direction)
		{
			/*if packet is already heading in one direction
			send it along in the same direction*/
			tx_port = (enum port_name)net_packet->direction;
		}
		else
		{
			/*determine the distance to the destination*/
			if(net_packet->dest[0] > __switch->id)
			{
				x = net_packet->dest[0] - __switch->id;
				y = NUMSWITCHES - x;
				tx_port = east_queue;
				net_packet->direction = east_queue;
				assert(x + y == NUMSWITCHES);
			}
			else if(net_packet->dest[0] < __switch->id)
			{
				x = __switch->id - net_packet->dest[0];
				y = NUMSWITCHES - x;
				tx_port = west_queue;
				net_packet->direction = west_queue;
				assert(x + y == NUMSWITCHES);
			}

			/*printf("x + y %d\n", x + y);*/

			//if the opposite direction is faster go that way
			if(x < y)
			{
				if(tx_port == east_queue)
				{
					tx_port = west_queue;
					net_packet->direction = west_queue;
				}
				else if(tx_port == west_queue)
				{
					tx_port = east_queue;
					net_packet->direction = east_queue;
				}
			}
		}
	}

	assert(tx_port >= 0 && tx_port < __switch->num_ports);
	return tx_port;
}

packet *switch_get_rx_packet(struct switch_t *__switch){

	packet *net_packet = NULL;

	//sanity check
	assert(__switch->current_queue >= 0 && __switch->current_queue < __switch->num_ports);

	switch(__switch->next_crossbar_lane)
	{
		case crossbar_request:
			net_packet = (packet *)desim_list_get(__switch->rx_request_queues[__switch->current_queue], 0);
			break;
		case crossbar_reply:
			net_packet = (packet *)desim_list_get(__switch->rx_reply_queues[__switch->current_queue], 0);
			break;
		case crossbar_invalid_lane:
		default:
			fatal("switch_get_rx_packet() Invalid port name\n");
			break;
	}

	return net_packet;
}

void switch_producer_ctrl(void){

	int my_pid = producer_pid++;

	int i = 0;
	int type = 0;
	int dest[2] = {0,0};

	packet *packet_ptr = NULL;

	//printf("producer_ctrl %d init\n", my_pid);
	while(i < NUMPACKETS)
	{
		//create packet, keep trying to place into swith rx queue
		type = rand() % 2; //random request or reply packet
		dest[0] = rand() % NUMSWITCHES; //random destination
		dest[1] = rand() % NUMPORTS;
		packet_ptr =  network_packet_create(type, dest); //create the packet and give it a size

		//requests
		if(packet_ptr->type == request)
		{
			while(desim_list_count(__switch[my_pid]->rx_request_queues[north_queue]) >= MAXQUEUEDEPTH)
			{
				printf("producer_ctrl %d: stalling rx request queue size %d cycle %llu\n",
						my_pid, desim_list_count(__switch[my_pid]->rx_request_queues[north_queue]), P_TIME);
				P_PAUSE(1);
			}

			//success clear the packet pointer
			desim_list_enqueue(__switch[my_pid]->rx_request_queues[north_queue], packet_ptr);
		}
		else if(packet_ptr->type == reply)
		{
			while(desim_list_count(__switch[my_pid]->rx_reply_queues[north_queue]) >= MAXQUEUEDEPTH)
			{
				printf("producer_ctrl %d: stalling rx reply queue size %d cycle %llu\n",
						my_pid, desim_list_count(__switch[my_pid]->rx_reply_queues[north_queue]), P_TIME);
				P_PAUSE(1);
			}

			//success clear the packet pointer
			desim_list_enqueue(__switch[my_pid]->rx_reply_queues[north_queue], packet_ptr);
		}

		packets_sent++;

		printf("producer_ctrl %d: success queue size %d cycle %llu\n",
		my_pid, desim_list_count(__switch[my_pid]->rx_request_queues[north_queue]), P_TIME);

		advance(switch_ec[my_pid]); //wait for entire messages to transfer before signaling the switch
		P_PAUSE(packet_ptr->size / 8); //transfer time
		packet_ptr = NULL;
		i++;
	}

	//This context finished its work, destroy
	printf("producer terminating cycle %llu\n", P_TIME);
	//getchar();
	context_terminate();

	return;
}

void network_packet_destroy(packet *net_packet){

	free(net_packet);
	return;
}

packet *network_packet_create(int type, int *dest){

	packet *new_packet = NULL;

	new_packet = (packet *) calloc((1), sizeof(packet));
	assert(new_packet);

	new_packet->id = packet_id++;

	//if 1 make packet a request packet
	new_packet->type = (enum type_name)type;
	new_packet->dest[0] = dest[0];
	new_packet->dest[1] = dest[1];

	//if its east or west change it for example purposes
	if(new_packet->dest[1] == east_queue || new_packet->dest[1] == west_queue)
		new_packet->dest[1]++;

	new_packet->direction = 0;

	if(type == request)
		new_packet->size = 8;
	else
		new_packet->size = 64;

	return new_packet;
}

void switch_producer_init(void){

	int i = 0;
	char buff[100];

	//for(i = 0; i < 1; i++)
	for(i = 0; i < NUMSWITCHES; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "producer_%d", i);
		context_create(switch_producer_ctrl, 32768, strdup(buff), i);
	}

	srand(time(NULL));

	return;
}

void switch_init(void){

	switch_create();
	switch_create_tasks();
	switch_create_io_tasks();

	return;
}

void switch_create_tasks(void){

	char buff[100];
	int i = 0;

	//create the user defined eventcounts
	switch_ec = (eventcount**) calloc(NUMSWITCHES, sizeof(eventcount*));
	for(i = 0; i < NUMSWITCHES; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "switch_ec_%d", i);
		switch_ec[i] = eventcount_create(strdup(buff));
	}

	for(i = 0; i < NUMSWITCHES; i++)
	{
		memset(buff,'\0' , 100);
		snprintf(buff, 100, "switch_ctrl_%d", i);
		context_create(switch_ctrl, 32768, strdup(buff), i);
	}

	return;
}

void switch_create_io_tasks(void){

	char buff[100];
	int i = 0;
	int j = 0;

	//create the IO controllers for each switch
	for(i = 0; i < NUMSWITCHES; i++)
	{
		__switch[i]->switch_io_ec = (eventcount**) calloc(NUMPORTS, sizeof(eventcount*));
		for(j = 0; j < NUMPORTS; j++)
		{
			memset(buff,'\0' , 100);
			snprintf(buff, 100, "switch_%d_io_ec_%d", __switch[i]->id, j);
			__switch[i]->switch_io_ec[j] = eventcount_create(strdup(buff));
		}

		for(j = 0; j < NUMPORTS; j++)
		{
			memset(buff,'\0' , 100);
			snprintf(buff, 100, "switch_%d_io_ctrl_%d", __switch[i]->id, j);
			context_create(switch_io_ctrl, 32768, strdup(buff), j);
		}
	}

	return;
}

void switch_create(void){

	char buff[100];
	int i = 0;
	int j = 0;

	//create the switch
	__switch = (struct switch_t **) calloc(NUMSWITCHES, sizeof(struct switch_t*));

	for(i = 0; i < NUMSWITCHES; i++)
	{
		__switch[i] = (struct switch_t *) malloc(sizeof(struct switch_t));

		memset (buff,'\0' , 100);
		snprintf(buff, 100, "switch[%d]", i);
		__switch[i]->name = strdup(buff);

		__switch[i]->id = i;

		__switch[i]->num_ports = NUMPORTS;

		__switch[i]->latency = SWITCHLATENCY;

		__switch[i]->arb_style = round_robin;

		__switch[i]->bus_width = BUSWIDTH;

		//create switch virtual queues...
		__switch[i]->rx_request_queues = (list **) calloc((__switch[i]->num_ports), sizeof(list *));
		__switch[i]->rx_reply_queues = (list **) calloc((__switch[i]->num_ports), sizeof(list *));
		__switch[i]->tx_request_queues = (list **) calloc((__switch[i]->num_ports), sizeof(list *));
		__switch[i]->tx_reply_queues = (list **) calloc((__switch[i]->num_ports), sizeof(list *));

		//allocate the queues...
		for(j = 0; j < __switch[i]->num_ports; j++)
		{
			__switch[i]->rx_request_queues[j] = desim_list_create(MAXQUEUEDEPTH);
			__switch[i]->rx_reply_queues[j] = desim_list_create(MAXQUEUEDEPTH);
			__switch[i]->tx_request_queues[j] = desim_list_create(MAXQUEUEDEPTH);
			__switch[i]->tx_reply_queues[j] = desim_list_create(MAXQUEUEDEPTH);
		}

		//create the crossbar
		__switch[i]->crossbar = switch_crossbar_create(__switch[i]);

		//init switch and crossbar pointers
		__switch[i]->current_queue = west_queue;
		__switch[i]->next_crossbar_lane = crossbar_request;
		__switch[i]->north_next_io_lane = io_request;
		__switch[i]->east_next_io_lane = io_request;
		__switch[i]->south_next_io_lane = io_request;
		__switch[i]->west_next_io_lane = io_request;
		__switch[i]->crossbar->current_port = west_queue;

		pthread_mutex_init(&__switch[i]->mutex, NULL);
	}

	//configure the ring bus
	for (i = 0; i < NUMSWITCHES; i++)
	{
		__switch[i]->node_number = __switch[i]->id;

		if(__switch[i]->id == 0)
		{
			//west queues
			__switch[i]->next_west = NUMSWITCHES - 1;
			__switch[i]->next_east = __switch[i]->id + 1;
		}
		else if(__switch[i]->id > 0 && __switch[i]->id < (NUMSWITCHES - 1))
		{
			__switch[i]->next_west = __switch[i]->id - 1;
			__switch[i]->next_east = __switch[i]->id + 1;
		}
		else
		{
			assert(__switch[i]->id == (NUMSWITCHES - 1));
			__switch[i]->next_west = __switch[i]->id - 1;
			__switch[i]->next_east = 0;
		}
		__switch[i]->next_west_rx_request_queue = __switch[__switch[i]->next_west]->rx_request_queues[east_queue];
		__switch[i]->next_west_rx_reply_queue = __switch[__switch[i]->next_west]->rx_reply_queues[east_queue];

		__switch[i]->next_east_rx_request_queue = __switch[__switch[i]->next_east]->rx_request_queues[west_queue];
		__switch[i]->next_east_rx_reply_queue = __switch[__switch[i]->next_east]->rx_reply_queues[west_queue];
	}

	return;
}
