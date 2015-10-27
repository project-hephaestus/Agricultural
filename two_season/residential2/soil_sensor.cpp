/** $Id: soil_sensor.cpp,v 1.51 2008/02/15 00:24:14 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file soil_sensor.cpp
	@addtogroup soil_sensor Electric soil_sensor
	@ingroup residential

	The residential electric soil_sensor uses a hybrid thermal model that is capable
	of tracking either a single-mass of water, or a dual-mass with a varying thermocline.

	The driving dynamic parameters of the soil_sensor are
	- <b>demand</b>: the current consumption of water in gallons/minute; the higher
	  the demand, the more quickly the thermocline drops.
	- <b>voltage</b>: the line voltage for the coil; the lower the voltage, the
	  more slowly the thermocline rises.
	- <b>inlet water temperature</b>: the inlet water temperature; the lower the inlet
	  water temperature, the more heat is needed to raise it to the set point
	- <b>indoor air temperature</b>: the higher the indoor temperature, the less heat
	  loss through the jacket.
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "soil_sensor.h"

#define TSTAT_PRECISION 0.01
#define HEIGHT_PRECISION 0.01

//////////////////////////////////////////////////////////////////////////
// soil_sensor CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* soil_sensor::oclass = NULL;
CLASS* soil_sensor::pclass = NULL;

/**  Register the class and publish water heater object properties
 **/


//int counter=600000;



soil_sensor::soil_sensor(MODULE *module) : residential_enduse(module){
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;
		// register the class definition
		oclass = gl_register_class(module,"soil_sensor",sizeof(soil_sensor),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			
			PT_double,"humidity",PADDR(humidity), PT_DESCRIPTION, "measured soil humidity",
			PT_double, "irrigation_level",PADDR(irrigation_level),
			PT_int16,"irrigate_flag",PADDR(irrigate_flag), PT_DESCRIPTION, "1 for increasing humidity",
			PT_int16,"force_irrigation",PADDR(force_irrigation), PT_DESCRIPTION, "if humidity too low force controller to be include in the market value 1, else value 0",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

soil_sensor::~soil_sensor()
{
}

int soil_sensor::create() 
{
	totaldt=0;
	first_time=0;
	dt=0;
	return 1;

}

/** Initialize water heater model properties - randomized defaults for all published variables
 **/
int soil_sensor::init(OBJECT *parent)
{
	next_run = gl_globalclock;
	counter=600000*(irrigation_level);
	counter2=600000*(irrigation_level);
	irrigate_flag = 0;
	parent2=parent;
	
	force_irrigation=0;

	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("soil_sensor::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	// check the load configuration before initializing the parent class
	
	FINDLIST *climates = NULL; 
		OBJECT *obj;
		climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		obj = gl_find_next(climates,NULL);

		if (gl_object_isa(obj,"climate"))
		{
			climate_object=obj;
		}


	return 1;
	
}


double soil_sensor::normalDistribution(double x)
{
  //if(x<-10.)return 0.;
  //if(x>10.)return 1.;
  // number of steps
  int N=2000;
  // range of integration
  double a=0,b=x;
  // local variables
  double s,h,sum=0.;
  // inialise the variables
  h=(b-a)/N;
  // add in the first few terms
  sum = sum + exp(-a*a/2.) + 4.*exp(-(a+h)*(a+h)/2.);
  // and the last one
  sum = sum + exp(-b*b/2.);
  // loop over terms 2 up to N-1
  for(int i=1;i<N/2;i++)
  {
    s = a + 2*i*h;
    sum = sum + 2.*exp(-s*s/2.);
    s = s + h;
    sum = sum + 4.*exp(-s*s/2.);
  }
  // complete the integral
  sum = 0.5 + h*sum/3./sqrt(8.*atan(1.));
  // return result
  return sum;
}

void soil_sensor::calculates_normal_ditribution(void)
{
	
	



	
}

int soil_sensor::isa(char *classname)
{
	return (strcmp(classname,"soil_sensor")==0 || residential_enduse::isa(classname));
}



/** Water heater plc control code to set the water heater 'heat_needed' state
	The thermostat set point, deadband, tank state(height of hot water column) and 
	current water temperature are used to determine 'heat_needed' state.
 **/
TIMESTAMP soil_sensor::presync(TIMESTAMP t0, TIMESTAMP t1){
	if(first_time==2)
	{

		//system("pause");
		double *hum_setpoint=gl_get_double_by_name(parent2,"humidity_setpoint");
	double 	*pSolarD = (double*)GETADDR(climate_object,gl_get_property(climate_object,"solar_flux"));
		int16 *crop=gl_get_int16_by_name(parent2,"crop_type");
	if(*pSolarD <= 0)
	{
	*pSolarD=2.5*(*crop);
	}
	if(irrigate_flag==0)
	{
			if(humidity<15)
			{
				counter=counter2;
			}
			else
			{
				counter=counter-1000*(*pSolarD);
			}

		
		humidity=normalDistribution(counter);
	//printf("humidity:%f	",humidity);
		if(humidity<15)
		{
		   counter2=counter;
		   return TS_NEVER;
		
		}
	
		// humidity=humidity-humidity*0.03;
	}
	}	
	return TS_NEVER;
	
}

/** Water heater synchronization determines the time to next
	synchronization state and the power drawn since last synch
 **/
TIMESTAMP soil_sensor::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP t2 = TS_NEVER;
	
	if(first_time==0)
	{
		first_time=t1;

	}

	char timebuf[128];
    gl_printtime(t0,timebuf,127);
   // printf("t0:simulation time: %s\n",timebuf);

	char timebuf2[128];
    gl_printtime(t1,timebuf2,127);
   // printf("t1:simulation time: %s\n",timebuf2);
//	printf("%f ",gl_todays(t1));
	if(gl_todays(t1)-gl_todays(first_time)==1)
	{
	first_time =2;
	
	}
	return t2;
}

TIMESTAMP soil_sensor::postsync(TIMESTAMP t0, TIMESTAMP t1){
	if(first_time==2)
	{
		int16 *crop=gl_get_int16_by_name(parent2,"crop_type");
		//printf("crop_type:%d\n ",*crop);
		if(irrigate_flag==1 ){
	
		
			if(humidity<70){
				counter=counter+200*(*crop);
			   humidity=normalDistribution(counter);
			  // printf("soil_%d %f\n\n\n",OBJECTHDR(this)->id,humidity);
			// humidity=humidity+humidity*0.03;
			}
			else{
			
				irrigate_flag=0;
			}

		}
	
	}

	 //totaldt+=dt;
	 
	 
	return TS_NEVER;
}



//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_soil_sensor(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(soil_sensor::oclass);
		if (*obj!=NULL)
		{
			soil_sensor *my = OBJECTDATA(*obj,soil_sensor);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(soil_sensor);
}

EXPORT int init_soil_sensor(OBJECT *obj)
{
	try
	{
		soil_sensor *my = OBJECTDATA(obj,soil_sensor);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(soil_sensor);
}

EXPORT int isa_soil_sensor(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,soil_sensor)->isa(classname);
	} else {
		return 0;
	}
}


EXPORT TIMESTAMP sync_soil_sensor(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		soil_sensor *my = OBJECTDATA(obj, soil_sensor);
		if (obj->clock <= ROUNDOFF)
			obj->clock = t0;  //set the object clock if it has not been set yet
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return my->presync(obj->clock, t0);
		case PC_BOTTOMUP:
			return my->sync(obj->clock, t0);
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock, t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
		return TS_INVALID;
	}
	SYNC_CATCHALL(soil_sensor);
}



/**@}**/
