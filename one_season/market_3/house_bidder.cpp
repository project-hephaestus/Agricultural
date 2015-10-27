#include "house_bidder.h"

CLASS* house_bidder::oclass = NULL;

house_bidder::house_bidder(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"house_bidder",sizeof(house_bidder),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
				PT_double, "bid_period[s]", PADDR(bid_period),
				PT_object, "parent2",PADDR(parent2),
				PT_int16, "count", PADDR(count),
				PT_object, "market", PADDR(market),
				PT_enumeration, "role", PADDR(role),
					PT_KEYWORD, "BUYER", (enumeration)BUYER,
					PT_KEYWORD, "SELLER", (enumeration)SELLER,
				PT_double, "price", PADDR(price),
				PT_double, "quantity", PADDR(quantity),
				NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
		
		memset(this,0,sizeof(house_bidder));
	}
}

int house_bidder::create()
{
	return SUCCESS;
}

int house_bidder::init(OBJECT *parent)
{
	new_bid_id = -1;
	next_t = 0;
	lastbid_id = -1;
	lastmkt_id = -1;
	quantity=1;
	//////////find climate object///////////////////
	FINDLIST *climates = NULL; 
		OBJECT *obj;
		climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);
		obj = gl_find_next(climates,NULL);

		if (gl_object_isa(obj,"climate"))
		{
			climate_object=obj;
		}




	/////////////////////////////////
	if (market==NULL)		
		throw "market is not defined";
	thismkt_id = (int64*)gl_get_addr(market,"market_id");
	if (thismkt_id==NULL)
		throw "market does not define market_id";
	return SUCCESS;
}

int house_bidder::isa(char *classname)
{
	return strcmp(classname,"house_bidder")==0;
}

TIMESTAMP house_bidder::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	double *bid_quantity=NULL;
	if (t1==next_t || next_t==0)
	{
		next_t=t1+(TIMESTAMP)bid_period;
		if(gl_object_isa(parent2,"house"))
		{
			bid_quantity=gl_get_double_by_name(parent2,"total_load");
			lastbid_id = (KEY)submit_bid(market,OBJECTHDR(this),role==BUYER?-(*bid_quantity):(*bid_quantity),price,*thismkt_id!=lastmkt_id?new_bid_id:lastbid_id);
		}
		else if(gl_object_isa(parent2,"virtual_battery"))
		{
			double *bid_q=gl_get_double_by_name(parent2,"capacity");
			double 	*pSolarD = (double*)GETADDR(climate_object,gl_get_property(climate_object,"solar_flux"));
		
		quantity=*bid_q/1000;
			lastbid_id = (KEY)submit_bid(market,OBJECTHDR(this),role==BUYER?-((*bid_q)/1000):((*bid_q)/1000),price,*thismkt_id!=lastmkt_id?new_bid_id:lastbid_id);
			
	//	printf("%f",*bid_q);
		//system("pause");
		}
		
		

		count--;
		lastmkt_id = *thismkt_id;
	}
	if (count>0)
		return next_t;
	else
		return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_house_bidder(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(house_bidder::oclass);
		if (*obj!=NULL)
		{
			house_bidder *my = OBJECTDATA(*obj,house_bidder);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_house_bidder: %s", msg);
		return 0;
	}
	return 1;
}

EXPORT int init_house_bidder(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,house_bidder)->init(parent);
		}
	}
	catch (const char *msg)
	{
		char name[64];
		gl_error("init_house_bidder(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
		return 0;
	}
	return 1;
}

EXPORT int isa_house_bidder(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,house_bidder)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_house_bidder(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	house_bidder *my = OBJECTDATA(obj,house_bidder);
	switch (pass) {
	case PC_BOTTOMUP:
		t2 = my->sync(obj->clock,t1);
		obj->clock = t1;
		break;
	default:
		break;
	}
	return t2;
}