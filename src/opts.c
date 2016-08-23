#include <stdlib.h>
#include <string.h>
#include <rpata.h>

#include "impl.h"

static bool ni_cmp(const char *ni1, const char *ni2)
{
	return 0 == strcmp(ni1, ni2);
}

static bool add_ni(struct rpata *ctx, char *ni)
{
	struct rpata_ipaddr *ips = ctx->ips;
	if(!ips)
		return false;

	bool ret = false;
	for(size_t i = 0; i < ips->nr_ips; ++i){
		if(ips->ips[i].addr && 
				ni_cmp(ni, ips->ips[i].name)){
			ctx->ni = strdup(ni);
			ret = true;
			break;
		}
	}

	return ret;
}

static bool set_mcastaddr(struct rpata *ctx, char *addr)
{
	if(ctx->mcast)
		free(ctx->mcast);

	ctx->mcast = strdup(addr);

	return true;
}

static bool set_mcastport(struct rpata *ctx, char *port)
{
	ctx->mcast_port = atoi(port);
	return true;
}

static bool set_mcastperiod(struct rpata *ctx, char *period)
{
	ctx->period = atoi(period);
	return true;
}

static bool set_timeout(struct rpata *ctx, char *to)
{
	ctx->timeout = atoi(to);
	return true;
}

bool rpata_setopt(struct rpata *ctx, int opt, char *val)
{
	if(ctx->start)
		return false;

	bool ret = false;
	switch(opt){
		case WITH_NI:
			ret = add_ni(ctx, val);
			break;
		case WITH_MCAST_ADDR:
			ret = set_mcastaddr(ctx, val);
			break;
		case WITH_MCAST_PORT:
			ret = set_mcastport(ctx, val);
			break;
		case WITH_PERIOD:
			ret = set_mcastperiod(ctx, val);
			break;
		case WITH_TIMEOUT:
			ret = set_timeout(ctx, val);
			break;
		default:
			ret = false;
	};

	return ret;
}
