#ifndef _house_bidder_h
#define _house_bidder_h

#include <stdarg.h>
#include "gridlabd.h"
#include "auction.h"

class house_bidder : public gld_object {
public:
	house_bidder(MODULE *);
	int create(void);
	OBJECT *parent2;
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	static CLASS *oclass;
public:
	double bid_period;
	OBJECT *climate_object;
	int16 count;
	object market;
	typedef enum {
		BUYER=0, 
		SELLER=1,
	} ROLE;
	enumeration role;
	double price;
	double quantity;
	int64 *thismkt_id;
private:
	int64 next_t;
	KEY new_bid_id;
	KEY lastbid_id;
	int64 lastmkt_id;
};

#endif //_house_bidder_h