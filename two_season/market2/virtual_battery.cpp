/** $Id: virtual_battery.cpp,v 1.51 2008/02/15 00:24:14 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file virtual_battery.cpp
	@addtogroup virtual_battery Electric virtual_battery
	@ingroup residential

	The residential electric virtual_battery uses a hybrid thermal model that is capable
	of tracking either a single-mass of water, or a dual-mass with a varying thermocline.

	The driving dynamic parameters of the virtual_battery are
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

#include "virtual_battery.h"

#define TSTAT_PRECISION 0.01
#define HEIGHT_PRECISION 0.01

//////////////////////////////////////////////////////////////////////////
// virtual_battery CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* virtual_battery::oclass = NULL;
CLASS* virtual_battery::pclass = NULL;

/**  Register the class and publish water heater object properties
 **/


//int counter=600000;



virtual_battery::virtual_battery(MODULE *module) {
	// first time init
	if (oclass==NULL)
	{
		
		// register the class definition
		oclass = gl_register_class(module,"virtual_battery",sizeof(virtual_battery),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			
			
			PT_double,"charge",PADDR(charge), PT_DESCRIPTION, "1 for charging the battery",
			PT_double,"price",PADDR(price), PT_DESCRIPTION, "virtual's battery priceS",
			PT_double,"capacity[W]",PADDR(capacity), PT_DESCRIPTION, "capacity ",
			PT_double,"max_capacity[W]",PADDR(max_capacity), PT_DESCRIPTION, "max cpapcity",
			PT_int16,"change_capacity",PADDR(change_capacity), PT_DESCRIPTION, "1 for change_capacity,0 for no change_capacity",
			PT_int16,"drilling_systems",PADDR(drilling_systems), PT_DESCRIPTION, "fgh",
		PT_int16,"ch",PADDR(ch), PT_DESCRIPTION, "fgh",
		
			
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

virtual_battery::~virtual_battery()
{
}

int virtual_battery::create() 
{
	charge=0;
	change_capacity=0;
	ch=0;
	max_capacity=100000;

	first=0;
	return 1;

}

/** Initialize water heater model properties - randomized defaults for all published variables
 **/
int virtual_battery::init(OBJECT *parent)
{
	
	parent2=parent;
	first_time=0;
	first_time2=0;
	actual=0;
	temp_capacity=0;
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("virtual_battery::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

		///////////////////////////////find auction object/////////////////////////////////////
				 static FINDLIST *xt1=NULL;
				 xt1=gl_find_objects(FL_NEW,FT_CLASS,SAME,"auction",FT_END);
				 OBJECT *firstt1= gl_find_next(xt1,NULL);
				 OBJECT *it1;
				 for(it1=firstt1;it1!=NULL;it1=it1->next)
				 {
				
					 if(gl_object_isa(it1,"auction"))
				     {

						
						 auction_object=it1;
						
					 }

				 }
       /////////////////////////////////////////////////////////////////////////////////////////
       /////////////////////////////find climate object ///////////////////////////////////////

		FINDLIST *climates = NULL; 
		OBJECT *obj;
		climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		obj = gl_find_next(climates,NULL);

		if (gl_object_isa(obj,"climate"))
		{
			climate_object=obj;
		}





      ///////////////////////////////////////////////////////////////////////////////////////	
	// check the load configuration before initializing the parent class
	
	return 1;
	
}



int virtual_battery::isa(char *classname)
{
	return (strcmp(classname,"virtual_battery")==0 );
}



/** Water heater plc control code to set the water heater 'heat_needed' state
	The thermostat set point, deadband, tank state(height of hot water column) and 
	current water temperature are used to determine 'heat_needed' state.
 **/
TIMESTAMP virtual_battery::presync(TIMESTAMP t0, TIMESTAMP t1){

		power= gl_get_double_by_name(parent2,"TotalRealPow");
	
			if(first_time==0)
			{
				first_time=t1;

			}

			if(gl_todays(t1)-gl_todays(first_time)==1)
			{
					first_time=2;
			}
			if(gl_todays(t1)-gl_todays(first_time)==2)
			{
				first_time2=2;
			}

	if(first_time==2 && first_time2==0)
	{
		if(change_capacity==1)
		{


		char timebuf[128];
		gl_printtime(t1,timebuf,127);
		printf("%s \n",timebuf);

		

		//system("pause");
			 double *clear_quantity=gl_get_double_by_name(auction_object,"current_market.clearing_quantity");
			 double *clear_price=gl_get_double_by_name(auction_object,"current_market.clearing_price");
			 if(*clear_quantity > capacity/1000)
			{
				// printf("1	glfdlhdljhdljkkjlhjlhlddfgdfg\n\n\n\n");
				  capacity=0;
				 
				// system("pause");
				 charge=1;
			 }
			 else		 if(*clear_price >= price)
			 {
				 //printf("2	");
				 capacity=0;

				// printf("post: %f ",capacity);
				// system("pause");
				
				 charge=1;
	 
			 }
			 else if(*clear_price < price)
			 {
				// printf("3	");
				 capacity=capacity;

			//  system("pause");
				 

				 charge=1;
	 
			 }
			 change_capacity=0;
	}
		//printf("%f\n\n", capacity);
	}
	
	return TS_NEVER;
	
}

/** Water heater synchronization determines the time to next
	synchronization state and the power drawn since last synch
 **/
TIMESTAMP virtual_battery::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP t2 = TS_NEVER;
	//printf("sync: %f ",capacity);
	double 	*pSolarD = (double*)GETADDR(climate_object,gl_get_property(climate_object,"solar_flux"));
		
	
	if(first_time==2 && first_time2==0)
	{	
		if(ch==1)
		{
					//system("pause");
					if(capacity >= 100000 )
					{

					//	printf("sync 1:%f	\n",capacity);
					 //   system("pause");
						charge=0;
	
					}
					if(capacity < max_capacity *0.2)
					{
	
		
					//	printf("sync 2:%f	\n",capacity);
					//	system("pause");
						charge=1;
					}


					//if(*pSolarD > 0)
					//{
							if(charge==1 )
							{
		
		    					//system("pause");
								capacity=capacity + (*power)/drilling_systems;
							//	printf("sync 3:%f	\n",capacity);
						//	system("pause");
		
							}
					//}
							//printf("%f\n\n", capacity);
		}
	ch=0;
	}
		
	return TS_NEVER;
}

TIMESTAMP virtual_battery::postsync(TIMESTAMP t0, TIMESTAMP t1){
	
	//printf("\n");
	
	return TS_NEVER;
}



//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_virtual_battery(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(virtual_battery::oclass);
		if (*obj!=NULL)
		{
			virtual_battery *my = OBJECTDATA(*obj,virtual_battery);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(virtual_battery);
}

EXPORT int init_virtual_battery(OBJECT *obj)
{
	try
	{
		virtual_battery *my = OBJECTDATA(obj,virtual_battery);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(virtual_battery);
}

EXPORT int isa_virtual_battery(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,virtual_battery)->isa(classname);
	} else {
		return 0;
	}
}


EXPORT TIMESTAMP sync_virtual_battery(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	virtual_battery *my = OBJECTDATA(obj,virtual_battery);
	try {
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			//obj->clock = t1;
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			//obj->clock = t1;
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			obj->clock = t1;
			break;
		default:
			gl_error("invalid pass request (%d)", pass);
			return TS_INVALID;
			break;
		}
		return t2;
	}
	SYNC_CATCHALL(virtual_battery);
}



/**@}**/
