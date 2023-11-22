#ifndef _SSTMAC_WAIT_H_
#define _SSTMAC_WAIT_H_

#include <rdma/fi_eq.h>
#include <ofi_list.h>

#ifdef __cplusplus
extern "C" {
#endif

int sstmac_wait_open(struct fid_fabric *fabric, struct fi_wait_attr *attr,
		   struct fid_wait **waitset);
int sstmac_wait_close(struct fid *wait);
int sstmac_wait_wait(struct fid_wait *wait, int timeout);

void sstmaci_add_wait(struct fid_wait *wait, struct fid *wait_obj);

#ifdef __cplusplus
}
#endif

#endif
