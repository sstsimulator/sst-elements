#ifndef _SSTMAC_POLL_H_
#define _SSTMAC_POLL_H_

#ifdef __cplusplus
extern "C" {
#endif

int sstmac_poll_poll(struct fid_poll *pollset, void **context, int count);

int sstmac_poll_add(struct fid_poll *pollset, struct fid *event_fid,
		  uint64_t flags);

int sstmac_poll_del(struct fid_poll *pollset, struct fid *event_fid,
		  uint64_t flags);

#ifdef __cplusplus
}
#endif

#endif
