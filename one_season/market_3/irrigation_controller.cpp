/** $Id: irrigation_controller.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file auction.cpp
	@defgroup irrigation_controller Transactive irrigation_controller, OlyPen experiment style
	@ingroup market

 **/

#include "irrigation_controller.h"

CLASS* irrigation_controller::oclass = NULL;

irrigation_controller::irrigation_controller(MODULE *module){
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"irrigation_controller",sizeof(irrigation_controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class irrigation_controller";
		else
			oclass->trl = TRL_QUALIFIED;

		if (gl_publish_variable(oclass,
			PT_int16, "insync", PADDR(insync),PT_DESCRIPTION, "0 not in sync, 1 in sync",
			PT_object,"soil_sensor",PADDR(soil_sensor),
			PT_enumeration, "simple_mode", PADDR(simplemode),
				PT_KEYWORD, "IRRIGATION_LOAD", (enumeration)SM_IRRIGATION_LOAD,
				PT_KEYWORD, "NONE", (enumeration)SM_NONE,
				PT_KEYWORD, "HOUSE_HEAT", (enumeration)SM_HOUSE_HEAT,
				PT_KEYWORD, "HOUSE_COOL", (enumeration)SM_HOUSE_COOL,
				PT_KEYWORD, "HOUSE_PREHEAT", (enumeration)SM_HOUSE_PREHEAT,
				PT_KEYWORD, "HOUSE_PRECOOL", (enumeration)SM_HOUSE_PRECOOL,
				PT_KEYWORD, "WATERHEATER", (enumeration)SM_WATERHEATER,
				PT_KEYWORD, "DOUBLE_RAMP", (enumeration)SM_DOUBLE_RAMP,
			PT_enumeration, "bid_mode", PADDR(bidmode),
				PT_KEYWORD, "ON", (enumeration)BM_ON,
				PT_KEYWORD, "OFF", (enumeration)BM_OFF,
			PT_enumeration, "use_override", PADDR(use_override),
				PT_KEYWORD, "OFF", (enumeration)OU_OFF,
				PT_KEYWORD, "ON", (enumeration)OU_ON,
			PT_double, "ramp_low[degF]", PADDR(ramp_low), PT_DESCRIPTION, "the comfort response below the setpoint",
			PT_double, "ramp_high[degF]", PADDR(ramp_high), PT_DESCRIPTION, "the comfort response above the setpoint",
			PT_double, "range_low", PADDR(range_low), PT_DESCRIPTION, "the setpoint limit on the low side",
			PT_double, "range_high", PADDR(range_high), PT_DESCRIPTION, "the setpoint limit on the high side",
			PT_char32, "target", PADDR(target), PT_DESCRIPTION, "the observed property (e.g., air temperature)",
			PT_char32, "setpoint", PADDR(setpoint), PT_DESCRIPTION, "the controlled property (e.g., heating setpoint)",
			PT_char32, "demand", PADDR(demand), PT_DESCRIPTION, "the controlled load when on",
			PT_char32, "load", PADDR(load), PT_DESCRIPTION, "the current controlled load",
			PT_char32, "total", PADDR(total), PT_DESCRIPTION, "the uncontrolled load (if any)",
			PT_object, "market", PADDR(pMarket), PT_DESCRIPTION, "the market to bid into",
			PT_char32, "state", PADDR(state), PT_DESCRIPTION, "the state property of the controlled load",
			PT_char32, "avg_target", PADDR(avg_target),
			PT_char32, "std_target", PADDR(std_target),
			PT_double, "bid_price", PADDR(last_p), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid price",
			PT_double, "bid_quantity", PADDR(last_q), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the bid quantity",
			PT_double, "set_temp[degF]", PADDR(set_temp), PT_ACCESS, PA_REFERENCE, PT_DESCRIPTION, "the reset value",
			PT_double, "base_setpoint[degF]", PADDR(setpoint0),
			// new stuff
			PT_double, "market_price", PADDR(clear_price), PT_DESCRIPTION, "the current market clearing price seen by the irrigation_controller.",
			PT_double, "period[s]", PADDR(dPeriod), PT_DESCRIPTION, "interval of time between market clearings",
			PT_enumeration, "control_mode", PADDR(control_mode),
				PT_KEYWORD, "RAMP", (enumeration)CN_RAMP,
				PT_KEYWORD, "DOUBLE_RAMP", (enumeration)CN_DOUBLE_RAMP,
			PT_enumeration, "resolve_mode", PADDR(resolve_mode),
				PT_KEYWORD, "DEADBAND", (enumeration)RM_DEADBAND,
				PT_KEYWORD, "SLIDING", (enumeration)RM_SLIDING,
			PT_double, "slider_setting",PADDR(slider_setting),
			PT_double, "slider_setting_heat", PADDR(slider_setting_heat),
			PT_double, "slider_setting_cool", PADDR(slider_setting_cool),
			PT_char32, "override", PADDR(re_override),
			// double ramp
			PT_double, "heating_range_high[degF]", PADDR(heat_range_high),
			PT_double, "heating_range_low[degF]", PADDR(heat_range_low),
			PT_double, "heating_ramp_high", PADDR(heat_ramp_high),
			PT_double, "heating_ramp_low", PADDR(heat_ramp_low),
			PT_double, "cooling_range_high[degF]", PADDR(cool_range_high),
			PT_double, "cooling_range_low[degF]", PADDR(cool_range_low),
			PT_double, "cooling_ramp_high", PADDR(cool_ramp_high),
			PT_double, "cooling_ramp_low", PADDR(cool_ramp_low),
			PT_double, "heating_base_setpoint[degF]", PADDR(heating_setpoint0),
			PT_double, "cooling_base_setpoint[degF]", PADDR(cooling_setpoint0),
			PT_char32, "deadband", PADDR(deadband),
			PT_char32, "heating_setpoint", PADDR(heating_setpoint),
			PT_char32, "heating_demand", PADDR(heating_demand),
			PT_char32, "cooling_setpoint", PADDR(cooling_setpoint),
			PT_char32, "cooling_demand", PADDR(cooling_demand),
			PT_double, "sliding_time_delay[s]", PADDR(sliding_time_delay), PT_DESCRIPTION, "time interval desired for the sliding resolve mode to change from cooling or heating to off",
			PT_bool, "use_predictive_bidding", PADDR(use_predictive_bidding),
			// redefinitions
			PT_char32, "average_target", PADDR(avg_target),
			PT_char32, "standard_deviation_target", PADDR(std_target),
#ifdef _DEBUG
			PT_enumeration, "current_mode", PADDR(thermostat_mode),
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "dominant_mode", PADDR(last_mode),
				PT_KEYWORD, "INVALID", (enumeration)TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_enumeration, "previous_mode", PADDR(previous_mode),
				PT_KEYWORD, "INVALID", TM_INVALID,
				PT_KEYWORD, "OFF", (enumeration)TM_OFF,
				PT_KEYWORD, "HEAT", (enumeration)TM_HEAT,
				PT_KEYWORD, "COOL", (enumeration)TM_COOL,
			PT_double, "heat_max", PADDR(heat_max),
			PT_double, "cool_min", PADDR(cool_min),
#endif
			PT_int32, "bid_delay", PADDR(bid_delay),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(irrigation_controller));
	}
}

int irrigation_controller::create(){
	memset(this, 0, sizeof(irrigation_controller));
	sprintf(avg_target.get_string(), "avg24");
	sprintf(std_target.get_string(), "std24");
	slider_setting_heat = -0.001;
	slider_setting_cool = -0.001;
	slider_setting = -0.001;
	sliding_time_delay = -1;
	lastbid_id = -1;
	use_override = OU_OFF;
	period = 0;
	first_period = 0;
	use_predictive_bidding = FALSE;
	return 1;
}

/** provides some easy default inputs for the transactive irrigation_controller,
	 and some examples of what various configurations would look like.
 **/
void irrigation_controller::cheat(){
	switch(simplemode){
		case SM_NONE:
			break;
		case SM_IRRIGATION_LOAD:
			sprintf(target, "humidity");
			sprintf(setpoint, "humidity_setpoint");
			sprintf(demand, "actual_power_non_zero");
			sprintf(total, "base_power");
			sprintf(load, "base_power");
		//	sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = 5;
			range_low = 1;
			range_high = 10;
			break;
		case SM_WATERHEATER:
			sprintf(target, "temperature");
			sprintf(setpoint, "tank_setpoint");
			sprintf(demand, "heating_element_capacity");
			sprintf(total, "actual_load");
			sprintf(load, "actual_load");
			sprintf(state, "power_state");
			ramp_low = -2;
			ramp_high = -2;
			range_low = 0;
			range_high = 10;
			break;
		default:
			break;
	}
}


/** convenience shorthand
 **/
void irrigation_controller::fetch(double **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "irrigation_controller:%i", hdr->id);
		if(*name == NULL)
			sprintf(msg, "%s: irrigation_controller unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: irrigation_controller unable to find %s", namestr, name);
		throw(msg);
	}
}

/** initialization process
 **/
int irrigation_controller::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	char tname[32];
	parent2=parent;
	insync=0;


	initial_zipload_power=gl_get_double_by_name(parent,"base_power");

	char *namestr = (hdr->name ? hdr->name : tname);

	sprintf(tname, "irrigation_controller:%i", hdr->id);
	first=0;
	cheat();

	if(parent == NULL){
		gl_error("%s: irrigation_controller has no parent, therefore nothing to control", namestr);
		return 0;
	}

	if(pMarket == NULL){
		gl_error("%s: irrigation_controller has no market, therefore no price signals", namestr);
		return 0;
	}

	if(gl_object_isa(pMarket, "auction")){
		gl_set_dependent(hdr, pMarket);
		market = OBJECTDATA(pMarket, auction);
	} else {
		gl_error("irrigation_controllers only work when attached to an 'auction' object");
		return 0;
	}

	if(dPeriod == 0.0){
		if((pMarket->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("irrigation_controller::init(): deferring initialization on %s", gl_name(pMarket, objname, 255));
			return 2; // defer
		}
		period = market->period;
	} else {
		period = (TIMESTAMP)floor(dPeriod + 0.5);
	}

	if(bid_delay < 0){
		bid_delay = -bid_delay;
	}
	if(bid_delay > period){
		gl_warning("Bid delay is greater than the irrigation_controller period. Resetting bid delay to 0.");
		bid_delay = 0;
	}

	if(target[0] == 0){
		GL_THROW("irrigation_controller: %i, target property not specified", hdr->id);
	}
	if(setpoint[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("irrigation_controller: %i, setpoint property not specified", hdr->id);;
	}
	if(demand[0] == 0 && control_mode == CN_RAMP){
		GL_THROW("irrigation_controller: %i, demand property not specified", hdr->id);
	}
	if(deadband[0] == 0 && use_predictive_bidding == TRUE && control_mode == CN_RAMP){
		GL_THROW("irrigation_controller: %i, deadband property not specified", hdr->id);
	}
	if(total[0] == 0){
		GL_THROW("irrigation_controller: %i, total property not specified", hdr->id);
	}
	if(load[0] == 0){
		GL_THROW("irrigation_controller: %i, load property not specified", hdr->id);
	}

	
	fetch(&pMonitor, target, parent); // auto tha einai to soil hmidit tha to pairnei apo to soil_SENSOR
	if(control_mode == CN_RAMP){
		fetch(&pSetpoint, setpoint, parent);
		fetch(&pDemand, demand, parent);
		fetch(&pTotal, total, parent);
		fetch(&pLoad, load, parent);
		if(use_predictive_bidding == TRUE){
			fetch(&pDeadband, deadband.get_string(), parent);
		}
	} 
	fetch(&pAvg, avg_target.get_string(), pMarket);
	fetch(&pStd, std_target.get_string(), pMarket);


	if(dir == 0){
		double high = ramp_high * range_high;
		double low = ramp_low * range_low;
			//printf("high:%f, low:%f, rh:%f, rl:%f,gh:%f,gl:%f\n\n\n\n",high,low,ramp_high,ramp_low,range_high,range_low);
		if(high > low){
			dir = 1;
		} else if(high < low){
			dir = -1;
		} else if((high == low) && (fabs(ramp_high) > 0.001 || fabs(ramp_low) > 0.001)){
			dir = 0;
			if(ramp_high > 0){
				direction = 1;
			} else {
				direction = -1;
			}
			gl_warning("%s: irrigation_controller has no price ramp", namestr);
			/* occurs given no price variation, or no control width (use a normal thermostat?) */
		}
		if(ramp_low * ramp_high < 0){
			gl_warning("%s: irrigation_controller price curve is not injective and may behave strangely");
			/* TROUBLESHOOT
				The price curve 'changes directions' at the setpoint, which may create odd
				conditions in a number of circumstances.
			 */
		}
	}
	if(setpoint0==0)
		setpoint0 = -1; // key to check first thing

//	double period = market->period;
//	next_run = gl_globalclock + (TIMESTAMP)(period - fmod(gl_globalclock+period,period));
	next_run = gl_globalclock;// + (market->period - gl_globalclock%market->period);
	init_time = gl_globalclock;
	time_off = TS_NEVER;
	if(sliding_time_delay < 0 )
		dtime_delay = 21600; // default sliding_time_delay of 6 hours
	else
		dtime_delay = (int64)sliding_time_delay;

	if(state[0] != 0){
		// grab state pointer
		pState = gl_get_enum_by_name(parent, state);
		last_pState = 0;
		if(pState == 0){
			gl_error("state property name \'%s\' is not published by parent class", state);
			return 0;
		}
	}

	
	// get override, if set
	if(re_override[0] != 0){
		pOverride = gl_get_enum_by_name(parent, re_override);
	}
	if((pOverride == 0) && (use_override == OU_ON)){
		gl_error("use_override is ON but no valid override property name is given");
		return 0;
	}

	if(control_mode == CN_RAMP){
		if(slider_setting < -0.001){
			gl_warning("slider_setting is negative, reseting to 0.0");
			slider_setting = 0.0;
		}
		if(slider_setting > 1.0){
			gl_warning("slider_setting is greater than 1.0, reseting to 1.0");
			slider_setting = 1.0;
		}
	}
	
	last_p = market->init_price;

	/////////////////search for virtual_battery///////////////////////
	/*
				 static FINDLIST *xt1=NULL;
				 xt1=gl_find_objects(FL_NEW,FT_CLASS,SAME,"virtual_battery",FT_END);
				 OBJECT *firstt1= gl_find_next(xt1,NULL);
				 OBJECT *it1;
				 for(it1=firstt1;it1!=NULL;it1=it1->next)
				 {
				
					 if(gl_object_isa(it1,"virtual_battery"))
				     {

						
						 virtual_battery_object=it1;
						
					 }
					 else
					 {
					 
					// virtual_battery_object=NULL;
					 
					 }


				 }

	*/
	//////////////////////////////////////////////////////////////////
	return 1;
}


int irrigation_controller::isa(char *classname)
{
	return strcmp(classname,"irrigation_controller")==0;
}


TIMESTAMP irrigation_controller::presync(TIMESTAMP t0, TIMESTAMP t1){



	//two different periods
	/*	if(first_time==0)
	    {
		first_time=t1;
			     }

			if(gl_todays(t1)-gl_todays(first_time)==3)
			{
				first_period=1;
				static FINDLIST *xt1=NULL;
				 xt1=gl_find_objects(FL_NEW,FT_CLASS,SAME,"controller",FT_END);
				 OBJECT *firstt1= gl_find_next(xt1,NULL);
				 OBJECT *it1;
				 for(it1=firstt1;it1!=NULL;it1=it1->next)
				 {
				
					 if(gl_object_isa(it1,"controller"))
				     {
						 gl_set_value_by_name(it1,"second_period_for_market","1")  ;
						 
				     }
			     }

	
			}
			*/
	if(slider_setting < -0.001)
		slider_setting = 0.0;
	if(slider_setting_heat < -0.001)
		slider_setting_heat = 0.0;
	if(slider_setting_cool < -0.001)
		slider_setting_cool = 0.0;
	if(slider_setting > 1.0)
		slider_setting = 1.0;
	if(slider_setting_heat > 1.0)
		slider_setting_heat = 1.0;
	if(slider_setting_cool > 1.0)
		slider_setting_cool = 1.0;

	if(control_mode == CN_RAMP && setpoint0 == -1)
		setpoint0 = *pSetpoint; // auto tha einai to orio gia to humidity to setpoint pou tha vazei o xristis
	


	if(control_mode == CN_RAMP){
		if (slider_setting == -0.001){
			min = setpoint0 + range_low;
			max = setpoint0 + range_high;
		} else if(slider_setting > 0){
			min = setpoint0 + range_low * slider_setting;
			max = setpoint0 + range_high * slider_setting;
			if(range_low != 0)
				ramp_low = 2 + (1 - slider_setting);
			else
				ramp_low = 0;
			if(range_high != 0)
				ramp_high = 2 + (1 - slider_setting);
			else
				ramp_high = 0;
		} else {
			min = setpoint0;
			max = setpoint0;
		}
	}
	if((thermostat_mode != TM_INVALID && thermostat_mode != TM_OFF) || t1 >= time_off)
		last_mode = thermostat_mode;
	else if(thermostat_mode == TM_INVALID)
		last_mode = TM_OFF;// this initializes last mode to off

	if(thermostat_mode != TM_INVALID)
		previous_mode = thermostat_mode;
	else
		previous_mode = TM_OFF;

	return TS_NEVER;
}

TIMESTAMP irrigation_controller::sync(TIMESTAMP t0, TIMESTAMP t1){
	double bid = -1.0;
	int64 no_bid = 0; // flag gets set when the current temperature drops in between the the heating setpoint and cooling setpoint curves
	double demand = 0.0;
	double rampify = 0.0;
	extern double bid_offset;
	double deadband_shift = 0.0;
	double shift_direction = 0.0;
	double shift_setpoint = 0.0;
	double prediction_ramp = 0.0;
	double prediction_range = 0.0;
	double midpoint = 0.0;
	OBJECT *hdr = OBJECTHDR(this);
	

if(insync==0)
{
	insync=2;
}
else if(insync==2)
{
	x=gl_get_double_by_name(parent2,"actual_power_non_zero");
//	printf("%d %f			 ",parent2->id,*x);
	//system("pause");

		insync=1;
		pDemand=x;
		initial_zipload_power=x;
}


	//if(first_period==0)  //for two diffrent periods
	//{

	
	//OBJECT *p=gl_get_object("sensor");
	// double *humidity=gl_get_double_by_name(p,"humidity");
	 double *humidity=gl_get_double_by_name(soil_sensor,"humidity");
		
		*pMonitor =*humidity;
		//printf("irrigaiton:%d %f	\n",soil_sensor->id,*pMonitor);
	//	system("pause");
		
	//printf("%f %f",*pMonitor,setpoint0);
	//system("pause");
	/* short circuit if the state variable doesn't change during the specified interval */
	if((t1 < next_run) && (market->market_id == lastmkt_id)){
		if(t1 <= next_run - bid_delay){
			if(use_predictive_bidding == TRUE && ((control_mode == CN_RAMP && last_setpoint != setpoint0) || (control_mode == CN_DOUBLE_RAMP && (last_heating_setpoint != heating_setpoint0 || last_cooling_setpoint != cooling_setpoint0)))) {
				;
			} else {// check to see if we have changed states
				if(pState == 0){
					return next_run;
				} else if(*pState == last_pState){
					return next_run;
				}
			}
		} else {
			return next_run;
		}
	}
	
	if(use_predictive_bidding == TRUE){
		deadband_shift = *pDeadband * 0.5;
	}

	if(control_mode == CN_RAMP){
		// if market has updated, continue onwards
		if(market->market_id != lastmkt_id){// && (*pAvg == 0.0 || *pStd == 0.0 || setpoint0 == 0.0)){
			//printf("EDWWWWWWWWWWWWWWWWWWW\n");
			//system("pause");
			lastmkt_id = market->market_id;
			lastbid_id = -1; // clear last bid id, refers to an old market
			// update using last price
			// T_set,a = T_set + (P_clear - P_avg) * | T_lim - T_set | / (k_T * stdev24)

			clear_price = market->current_frame.clearing_price;

			if(use_predictive_bidding == TRUE){
				if((dir > 0 && clear_price < last_p) || (dir < 0 && clear_price > last_p)){
					shift_direction = -1;
				} else if((dir > 0 && clear_price >= last_p) || (dir < 0 && clear_price <= last_p)){
					shift_direction = 1;
				} else {
					shift_direction = 0;
				}
			}
			if(fabs(*pStd) < bid_offset){
				set_temp = setpoint0;
			} else if(clear_price < *pAvg && range_low != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_low) / (ramp_low * *pStd) + deadband_shift*shift_direction;
			} else if(clear_price > *pAvg && range_high != 0){
				set_temp = setpoint0 + (clear_price - *pAvg) * fabs(range_high) / (ramp_high * *pStd) + deadband_shift*shift_direction;
			} else {
				set_temp = setpoint0 + deadband_shift*shift_direction;
			}

			if((use_override == OU_ON) && (pOverride != 0)){
				if(clear_price <= last_p){
					// if we're willing to pay as much as, or for more than the offered price, then run.
					*pOverride = 1;
				} else {
					*pOverride = -1;
				}
			}

			// clip
			if(set_temp > max){
				set_temp = max;
			} else if(set_temp < min){
				set_temp = min;
			}

			*pSetpoint = set_temp;
			//gl_verbose("irrigation_controller::postsync(): temp %f given p %f vs avg %f",set_temp, market->next.price, market->avg24);
		}
		
		if(dir > 0){
				//edw mpainei
			if(use_predictive_bidding == TRUE){
				if(*pState == 0 && *pMonitor > (max - deadband_shift)){
					bid = market->pricecap;
				} else if(*pState != 0 && *pMonitor < (min + deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(*pState != 0 && *pMonitor > max){
					bid = market->pricecap;
				} else if(*pState == 0 && *pMonitor < min){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor > max){
			//		printf("sto max");
				
					bid = market->pricecap;
				} else if (*pMonitor < min){
				//	printf("sto min");
					
					bid = -1.0;
					no_bid = 0;
				}
			}
		} else if(dir < 0){
		
				
			if(use_predictive_bidding == TRUE){
				if(*pState == 0 && *pMonitor < (min + deadband_shift)){
					bid = market->pricecap;
				} else if(*pState != 0 && *pMonitor > (max - deadband_shift)){
					bid = 0.0;
					no_bid = 1;
				} else if(*pState != 0 && *pMonitor < min){
					bid = market->pricecap;
				} else if(*pState == 0 && *pMonitor > max){
					bid = 0.0;
					no_bid = 1;
				}
			} else {
				if(*pMonitor < min){
					bid = market->pricecap;
				} else if (*pMonitor > max){
					bid = 0.0;
					no_bid = 0;
				}
			}
		} else if(dir == 0){

				
			if(use_predictive_bidding == TRUE){
				if(direction == 0.0) {
					gl_error("the variable direction did not get set correctly.");
				} else if((*pMonitor > max + deadband_shift || (*pState != 0 && *pMonitor > min - deadband_shift)) && direction > 0){
					bid = market->pricecap;
				} else if((*pMonitor < min - deadband_shift || (*pState != 0 && *pMonitor < max + deadband_shift)) && direction < 0){
					bid = market->pricecap;
				} else {
					bid = 0.0;
					no_bid = 0;
				}
			} else {
				if(*pMonitor < min){
					bid = market->pricecap;
				} else if(*pMonitor > max){
					bid = 0.0;
					no_bid = 0;
				} else {
					bid = *pAvg;
				}
			}
		}
		
		// calculate bid price
	//printf("%f,monitor:%f,min:%f max:%f, setpoint:%f\n",*humidity,*pMonitor,min,max,setpoint0);
		if(*pMonitor > setpoint0){
			k_T = ramp_low;
			T_lim = range_low;
			bid=0;
			
		  // printf("values : %f %f %f \n",bid,k_T, T_lim );
		} else if(*pMonitor < setpoint0) {
			//printf("right_side ");
		    
			k_T = ramp_low;
			T_lim = range_low;
			//printf("values : %f %f %f \n",bid,k_T, T_lim );

			///////////////////close all the controllers////////////////////////////
						
				static FINDLIST *xt1=NULL;
				 xt1=gl_find_objects(FL_NEW,FT_CLASS,SAME,"controller",FT_END);
				 OBJECT *firstt1= gl_find_next(xt1,NULL);
				 OBJECT *it1;
				 for(it1=firstt1;it1!=NULL;it1=it1->next)
				 {
				
					 if(gl_object_isa(it1,"controller"))
				     {
						 gl_set_value_by_name(it1,"second_period_for_market","1")  ;
						 
				     }
			     }


		//////////////////////////////////////////////////////////////////////////////////
		} else {
			
			k_T = 0.0;
			T_lim = 0.0;
		}
	
			
		if(bid < 0.0 && *pMonitor != setpoint0) {		
			
			gl_set_value_by_name(soil_sensor,"irrigate_flag","1");
			last_q = *initial_zipload_power;
			*pDemand =*initial_zipload_power;
			
			bid = *pAvg + ( (fabs(*pStd) < bid_offset) ? 0.0 : (*pMonitor - setpoint0) * (k_T * *pStd) / fabs(T_lim) );
			//printf("price:%f %f %f %f\n",bid,(*pMonitor - setpoint0) ,(k_T * *pStd) , fabs(T_lim));
				
	
			//////////////////////////////////////
			char x_position_string[1024];
			double *prev=gl_get_double_by_name(parent2,"prev_base_power");
		    double pos_x = *prev;
			sprintf(x_position_string, "%f", pos_x);
			gl_set_value_by_name(parent2,"base_power",x_position_string);
			/////////////////////////////////////

		} else if(*pMonitor == setpoint0) {
			bid = *pAvg;
			
		}
		else
		{
		    last_q=0;
			
			gl_set_value_by_name(parent2,"base_power","0");
		}

		// bid the response part of the load
		double residual = *pTotal;
		/* WARNING ~ bid ID check will not work properly */
		KEY bid_id = (KEY)(lastmkt_id == market->market_id ? lastbid_id : -1);
		// override
		//bid_id = -1;
		
		if(last_q > 0 && no_bid != 1){
			
			last_p = bid;
			last_q= *initial_zipload_power;
		
			//if(last_p < 0)
			//{
				//last_p=clear_price;
			//}
			if(0 != strcmp(market->unit, "")){
				if(0 == gl_convert("kW", market->unit, &(last_q))){
					gl_error("unable to convert bid units from 'kW' to '%s'", market->unit.get_string());
					return TS_INVALID;
				}
			}
			//lastbid_id = market->submit(OBJECTHDR(this), -last_q, last_p, bid_id, (BIDDERSTATE)(pState != 0 ? *pState : 0));
			if(pState != 0){
				
				lastbid_id = submit_bid_state(pMarket, hdr, -last_q, last_p, (*pState > 0 ? 1 : 0), bid_id);
			} else {
				
				lastbid_id = submit_bid(pMarket, hdr, -last_q, last_p, bid_id);
			}
			residual -= *pLoad;

		} else {
			last_p = 0;
			last_q = 0;
			gl_verbose("%s's is not bidding", hdr->name);
		}
		if(residual < -0.001)
			gl_warning("irrigation_controller:%d: residual unresponsive load is negative! (%.1f kW)", hdr->id, residual);
	} 

	if (pState != 0)
		last_pState = *pState;

	
	char timebuf[128];
	gl_printtime(t1,timebuf,127);
	

	return TS_NEVER;


	//}
	
	//} //end of first_period==0
}

TIMESTAMP irrigation_controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	TIMESTAMP rv = next_run - bid_delay;

	if(last_setpoint != setpoint0 && control_mode == CN_RAMP){
		last_setpoint = setpoint0;
	}
	// Determine the system_mode the HVAC is in
	if(t1 < next_run-bid_delay){
		return next_run-bid_delay;
	}

	if (t1 - next_run < bid_delay){
		rv = next_run;
	}

	if(next_run == t1){
		next_run += (TIMESTAMP)(this->period);
		rv = next_run - bid_delay;
	}





	return rv;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_irrigation_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(irrigation_controller::oclass);
		if (*obj!=NULL)
		{
			irrigation_controller *my = OBJECTDATA(*obj,irrigation_controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(irrigation_controller);
}

EXPORT int init_irrigation_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
		{
			return OBJECTDATA(obj,irrigation_controller)->init(parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(irrigation_controller);
}

EXPORT int isa_irrigation_controller(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,irrigation_controller)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_irrigation_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	irrigation_controller *my = OBJECTDATA(obj,irrigation_controller);
	try
	{
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
	SYNC_CATCHALL(irrigation_controller);
}

// EOF
