/** $Id: virtual_battery.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file virtual_battery.h
	@addtogroup virtual_battery
	@ingroup residential

 @{
 **/

#ifndef _virtual_battery_H
#define _virtual_battery_H

#include "gridlabd.h"


class virtual_battery {

public:
	int first_time;
	OBJECT *parent2;
	OBJECT *auction_object;
	OBJECT *climate_object;
	double charge;
	double capacity;
	double temp_capacity;
	double max_capacity;
	double price;
	double *power;
	double actual;
	int16 drilling_systems;
     int first;
	int16 change_capacity;
	int16 ch;
////////////////////
public:
	static CLASS *oclass, *pclass;
	static virtual_battery *defaults;
	
	virtual_battery(MODULE *mod);
	~virtual_battery(void);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	void thermostat(TIMESTAMP t0, TIMESTAMP t1);					// Thermostat plc control code - determines whether to heat...
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	



};

#endif

/**@}**/
