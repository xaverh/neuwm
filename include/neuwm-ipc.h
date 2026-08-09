#ifndef NEUWM_IPC_H
#define NEUWM_IPC_H

void ipc_init(void);
void ipc_finish(void);
void ipc_emit_ws(void);
void ipc_emit_mode(void);
void ipc_emit_focused_geom(void);

#endif
