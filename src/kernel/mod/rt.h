#ifndef FSLRT_H
#define FSLRT_H

int kfsl_rt_init_bdev(struct block_device* bdev);
int kfsl_rt_init_fd(unsigned long fd);
int kfsl_rt_uninit(void);
bool kfsl_rt_inited(void);

#endif
