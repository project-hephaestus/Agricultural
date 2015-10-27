/** $Id: soil_sensor.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file soil_sensor.h
	@addtogroup soil_sensor
	@ingroup residential

 @{
 **/

#ifndef _soil_sensor_H
#define _soil_sensor_H

#include "residential.h"
#include "residential_enduse.h"

class soil_sensor : public residential_enduse {

public:
	double humidity;
	OBJECT *parent2;
	OBJECT *climate_object;
	double force_irrigation;
	int16 irrigate_flag;
	OBJECT *auction_object;
	OBJECT *controller_object;
	double lastmrk_id;
	double irrigation_level;
	 double dt;
	 double totaldt;
	 double normalDistribution(double x);
	 int first_time;
	 int counter;
	 int counter2;
	 TIMESTAMP next_run;
////////////////////
public:
	static CLASS *oclass, *pclass;
	static soil_sensor *defaults;
	void calculates_normal_ditribution();
	soil_sensor(MODULE *mod);
	~soil_sensor(void);
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
