#ifndef _CONFIG_H_
#define ifndef _CONFIG_H_

int parse_cfg(void);
void __cbTimerParseCfg(struct ev_loop *loop, struct ev_timer *w, int revents);
int pickOneRichRobot(int vid);

#endif
