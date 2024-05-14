// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sumi_prov.h"

EXTERN_C DIRECT_FN STATIC  int sumi_cntr_wait(struct fid_cntr *cntr, uint64_t threshold,
				    int timeout);
EXTERN_C DIRECT_FN STATIC  int sumi_cntr_adderr(struct fid_cntr *cntr, uint64_t value);
EXTERN_C DIRECT_FN STATIC  int sumi_cntr_seterr(struct fid_cntr *cntr, uint64_t value);
DIRECT_FN STATIC uint64_t sumi_cntr_readerr(struct fid_cntr *cntr);
EXTERN_C DIRECT_FN STATIC  int sumi_cntr_seterr(struct fid_cntr *cntr, uint64_t value);
DIRECT_FN STATIC uint64_t sumi_cntr_read(struct fid_cntr *cntr);
EXTERN_C DIRECT_FN STATIC  int sumi_cntr_add(struct fid_cntr *cntr, uint64_t value);
extern "C" DIRECT_FN  int sumi_cntr_open(struct fid_domain *domain,
			     struct fi_cntr_attr *attr,
			     struct fid_cntr **cntr, void *context);
static int sumi_cntr_control(struct fid *cntr, int command, void *arg);
static int sumi_cntr_close(fid_t fid);
EXTERN_C DIRECT_FN STATIC  int sumi_cntr_set(struct fid_cntr *cntr, uint64_t value);

static struct fi_ops sumi_cntr_fi_ops = {
  .size = sizeof(struct fi_ops),
  .close = sumi_cntr_close,
  .bind = fi_no_bind,
  .control = sumi_cntr_control,
  .ops_open = fi_no_ops_open,
};

static struct fi_ops_cntr sumi_cntr_ops = {
  .size = sizeof(struct fi_ops_cntr),
  .read = sumi_cntr_read,
  .readerr = sumi_cntr_readerr,
  .add = sumi_cntr_add,
  .set = sumi_cntr_set,
  .wait = sumi_cntr_wait,
  .adderr = sumi_cntr_adderr,
  .seterr = sumi_cntr_seterr
};

EXTERN_C DIRECT_FN STATIC  int sumi_cntr_wait(struct fid_cntr *cntr, uint64_t threshold,
				    int timeout)
{
#if 0
  struct sumi_fid_cntr *cntr_priv;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);
	if (!cntr_priv->wait)
		return -FI_EINVAL;

	if (cntr_priv->attr.wait_obj == FI_WAIT_SET ||
	    cntr_priv->attr.wait_obj == FI_WAIT_NONE)
		return -FI_EINVAL;

  return sumi_cntr_wait_sleep(cntr_priv, threshold, timeout);
#endif
  return 0;
}

EXTERN_C DIRECT_FN STATIC  int sumi_cntr_adderr(struct fid_cntr *cntr, uint64_t value)
{
#if 0
  struct sumi_fid_cntr *cntr_priv;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);
	if (FI_VERSION_LT(cntr_priv->domain->fabric->fab_fid.api_version, FI_VERSION(1, 5)))
		return -FI_EOPNOTSUPP;

	ofi_atomic_add32(&cntr_priv->cnt_err, (int)value);

	if (cntr_priv->wait)
    _sumi_signal_wait_obj(cntr_priv->wait);
#endif
	return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_cntr_seterr(struct fid_cntr *cntr, uint64_t value)
{
#if 0
  struct sumi_fid_cntr *cntr_priv;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);

	if (FI_VERSION_LT(cntr_priv->domain->fabric->fab_fid.api_version, FI_VERSION(1, 5)))
		return -FI_EOPNOTSUPP;

	ofi_atomic_set32(&cntr_priv->cnt_err, (int)value);

	if (cntr_priv->wait)
    _sumi_signal_wait_obj(cntr_priv->wait);
#endif
	return FI_SUCCESS;
}

static int sumi_cntr_close(fid_t fid)
{
#if 0
  struct sumi_fid_cntr *cntr;
	int references_held;

  SUMI_TRACE(FI_LOG_CQ, "\n");

  cntr = container_of(fid, struct sumi_fid_cntr, cntr_fid.fid);

	/* applications should never call close more than once. */
  references_held = _sumi_ref_put(cntr);
	if (references_held) {
    SUMI_INFO(FI_LOG_CQ, "failed to fully close cntr due to lingering "
			  "references. references=%i cntr=%p\n",
			  references_held, cntr);
	}
#endif
	return FI_SUCCESS;
}

DIRECT_FN STATIC uint64_t sumi_cntr_readerr(struct fid_cntr *cntr)
{
	int v, ret;
  struct sumi_fid_cntr *cntr_priv;
#if 0

	if (cntr == NULL)
		return -FI_EINVAL;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);
	v = ofi_atomic_get32(&cntr_priv->cnt_err);

  ret = __sumi_cntr_progress(cntr_priv);
	if (ret != FI_SUCCESS)
    SUMI_WARN(FI_LOG_CQ, " __sumi_cntr_progress returned %d.\n",
			  ret);
#endif
	return (uint64_t)v;
}

DIRECT_FN STATIC uint64_t sumi_cntr_read(struct fid_cntr *cntr)
{
	int v, ret;
#if 0
  struct sumi_fid_cntr *cntr_priv;

	if (cntr == NULL)
		return -FI_EINVAL;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);

	if (cntr_priv->wait)
    sumi_wait_wait((struct fid_wait *)cntr_priv->wait, 0);

  ret = __sumi_cntr_progress(cntr_priv);
	if (ret != FI_SUCCESS)
    SUMI_WARN(FI_LOG_CQ, " __sumi_cntr_progress returned %d.\n",
			  ret);

	v = ofi_atomic_get32(&cntr_priv->cnt);
#endif
	return (uint64_t)v;
}

EXTERN_C DIRECT_FN STATIC  int sumi_cntr_add(struct fid_cntr *cntr, uint64_t value)
{
#if 0
  struct sumi_fid_cntr *cntr_priv;

	if (cntr == NULL)
		return -FI_EINVAL;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);
	ofi_atomic_add32(&cntr_priv->cnt, (int)value);

	if (cntr_priv->wait)
    _sumi_signal_wait_obj(cntr_priv->wait);

  _sumi_trigger_check_cntr(cntr_priv);
#endif
	return FI_SUCCESS;
}

EXTERN_C DIRECT_FN STATIC  int sumi_cntr_set(struct fid_cntr *cntr, uint64_t value)
{
#if 0
  struct sumi_fid_cntr *cntr_priv;

	if (cntr == NULL)
		return -FI_EINVAL;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);
	ofi_atomic_set32(&cntr_priv->cnt, (int)value);

	if (cntr_priv->wait)
    _sumi_signal_wait_obj(cntr_priv->wait);

  _sumi_trigger_check_cntr(cntr_priv);
#endif
	return FI_SUCCESS;
}

static int sumi_cntr_control(struct fid *cntr, int command, void *arg)
{
#if 0
  struct sumi_fid_cntr *cntr_priv;

	if (cntr == NULL)
		return -FI_EINVAL;

  cntr_priv = container_of(cntr, struct sumi_fid_cntr, cntr_fid);

	switch (command) {
	case FI_SETOPSFLAG:
		cntr_priv->attr.flags = *(uint64_t *)arg;
		break;
	case FI_GETOPSFLAG:
		if (!arg)
			return -FI_EINVAL;
		*(uint64_t *)arg = cntr_priv->attr.flags;
		break;
	case FI_GETWAIT:
    /* return _sumi_get_wait_obj(cntr_priv->wait, arg); */
		return -FI_ENOSYS;
	default:
		return -FI_EINVAL;
	}
#endif
	return FI_SUCCESS;

}



extern "C" DIRECT_FN  int sumi_cntr_open(struct fid_domain *domain,
			     struct fi_cntr_attr *attr,
			     struct fid_cntr **cntr, void *context)
{
	int ret = FI_SUCCESS;
#if 0
  struct sumi_fid_domain *domain_priv;
  struct sumi_fid_cntr *cntr_priv;

  SUMI_TRACE(FI_LOG_CQ, "\n");

	ret = __verify_cntr_attr(attr);
	if (ret)
		goto err;

  domain_priv = container_of(domain, struct sumi_fid_domain, domain_fid);
	if (!domain_priv) {
		ret = -FI_EINVAL;
		goto err;
	}

  cntr_priv = (sumi_fid_cntr*) calloc(1, sizeof(*cntr_priv));
	if (!cntr_priv) {
		ret = -FI_ENOMEM;
		goto err;
	}

	cntr_priv->requires_lock = (domain_priv->thread_model !=
			FI_THREAD_COMPLETION);

	cntr_priv->domain = domain_priv;
	cntr_priv->attr = *attr;
	/* ref count is initialized to one to show that the counter exists */
  _sumi_ref_init(&cntr_priv->ref_cnt, 1, __cntr_destruct);

	/* initialize atomics */
	ofi_atomic_initialize32(&cntr_priv->cnt, 0);
	ofi_atomic_initialize32(&cntr_priv->cnt_err, 0);

  _sumi_ref_get(cntr_priv->domain);

  _sumi_prog_init(&cntr_priv->pset);

	dlist_init(&cntr_priv->trigger_list);
	fastlock_init(&cntr_priv->trigger_lock);

  ret = sumi_cntr_set_wait(cntr_priv);
	if (ret)
		goto err_wait;

	cntr_priv->cntr_fid.fid.fclass = FI_CLASS_CNTR;
	cntr_priv->cntr_fid.fid.context = context;
  cntr_priv->cntr_fid.fid.ops = &sumi_cntr_fi_ops;
  cntr_priv->cntr_fid.ops = &sumi_cntr_ops;

	*cntr = &cntr_priv->cntr_fid;
	return ret;

err_wait:
  _sumi_ref_put(cntr_priv->domain);
	free(cntr_priv);
#endif
err:
	return ret;
}



