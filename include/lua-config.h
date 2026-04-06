#ifndef LUA_CONFIG_H
#define LUA_CONFIG_H

#include <stddef.h>
#include "types.h"

struct axis_bind {
	uint32_t  mods;
	void    (*fn)(void *data, uint32_t time, uint32_t axis, int32_t value120);
};

extern struct config     cfg;
extern struct bind      *binds;
extern size_t            nbinds;
extern struct axis_bind *axis_binds;
extern size_t            naxis_binds;
extern char            **termcmd;
extern char            **autostart_cmds;
extern size_t            autostart_count;

void load_config(void);

#endif /* LUA_CONFIG_H */
