/*
 * Copyright (c) 2016-2017 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//#include "config.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ofi_shm.h>

struct dlist_entry ep_name_list;

DEFINE_LIST(ep_name_list);

void smr_cleanup(void)
{
	struct smr_ep_name *ep_name;
	struct dlist_entry *tmp;

	dlist_foreach_container_safe(&ep_name_list, struct smr_ep_name,
				     ep_name, entry, tmp)
		free(ep_name);
}

static void smr_peer_addr_init(struct smr_addr *peer)
{
	memset(peer->name, 0, NAME_MAX);
	peer->addr = FI_ADDR_UNSPEC;
}

/* TODO: Determine if aligning SMR data helps performance */
int smr_create(const struct fi_provider *prov, struct smr_map *map,
	       const struct smr_attr *attr, struct smr_region **smr)
{
	struct smr_ep_name *ep_name;
	size_t total_size, cmd_queue_offset, peer_addr_offset;
	size_t resp_queue_offset, inject_pool_offset, name_offset;
	int fd, ret, i;
	void *mapped_addr;
	size_t tx_size, rx_size;

	cmd_queue_offset = sizeof(**smr);

	tx_size = roundup_power_of_two(attr->tx_count);
	rx_size = roundup_power_of_two(attr->rx_count);
	resp_queue_offset = cmd_queue_offset + sizeof(struct smr_cmd_queue) +
			sizeof(struct smr_cmd) * rx_size;
	inject_pool_offset = resp_queue_offset + sizeof(struct smr_resp_queue) +
			sizeof(struct smr_resp) * tx_size;
	peer_addr_offset = inject_pool_offset + sizeof(struct smr_inject_pool) +
			sizeof(struct smr_inject_pool_entry) * rx_size;
	name_offset = peer_addr_offset + sizeof(struct smr_addr) * SMR_MAX_PEERS;
	total_size = name_offset + strlen(attr->name) + 1;
	total_size = roundup_power_of_two(total_size);

	fd = shm_open(attr->name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		FI_WARN(prov, FI_LOG_EP_CTRL, "shm_open error\n");
		goto err1;
	}

	ep_name = calloc(1, sizeof(*ep_name));
	if (!ep_name) {
		FI_WARN(prov, FI_LOG_EP_CTRL, "calloc error\n");
		return -FI_ENOMEM;
	}
	strncpy(ep_name->name, (char *)attr->name, NAME_MAX - 1);
	ep_name->name[NAME_MAX - 1] = '\0';
	dlist_insert_tail(&ep_name->entry, &ep_name_list);

	ret = ftruncate(fd, total_size);
	if (ret < 0) {
		FI_WARN(prov, FI_LOG_EP_CTRL, "ftruncate error\n");
		goto err2;
	}

	mapped_addr = mmap(NULL, total_size, PROT_READ | PROT_WRITE,
			   MAP_SHARED, fd, 0);
	if (mapped_addr == MAP_FAILED) {
		FI_WARN(prov, FI_LOG_EP_CTRL, "mmap error\n");
		goto err2;
	}

	close(fd);

	*smr = mapped_addr;
	fastlock_init(&(*smr)->lock);
	fastlock_acquire(&(*smr)->lock);

	(*smr)->map = map;
	(*smr)->version = SMR_VERSION;
	(*smr)->flags = SMR_FLAG_ATOMIC | SMR_FLAG_DEBUG;
	(*smr)->pid = getpid();

	(*smr)->total_size = total_size;
	(*smr)->cmd_queue_offset = cmd_queue_offset;
	(*smr)->resp_queue_offset = resp_queue_offset;
	(*smr)->inject_pool_offset = inject_pool_offset;
	(*smr)->peer_addr_offset = peer_addr_offset;
	(*smr)->name_offset = name_offset;
	(*smr)->cmd_cnt = rx_size;

	smr_cmd_queue_init(smr_cmd_queue(*smr), rx_size);
	smr_resp_queue_init(smr_resp_queue(*smr), tx_size);
	smr_inject_pool_init(smr_inject_pool(*smr), rx_size);
	for (i = 0; i < SMR_MAX_PEERS; i++)
		smr_peer_addr_init(&smr_peer_addr(*smr)[i]);

	strncpy((char *) smr_name(*smr), attr->name, total_size - name_offset);
	fastlock_release(&(*smr)->lock);

	return 0;

err2:
	shm_unlink(attr->name);
	close(fd);
err1:
	return -errno;
}

void smr_free(struct smr_region *smr)
{
	shm_unlink(smr_name(smr));
	munmap(smr, smr->total_size);
}

int smr_map_create(const struct fi_provider *prov, int peer_count,
		   struct smr_map **map)
{
	int i;

	(*map) = calloc(1, sizeof(struct smr_map));
	if (!*map) {
		FI_WARN(prov, FI_LOG_DOMAIN, "failed to create SHM region group\n");
		return -FI_ENOMEM;
	}

	for (i = 0; i < peer_count; i++)
		smr_peer_addr_init(&(*map)->peers[i].peer);

	fastlock_init(&(*map)->lock);

	return 0;
}

int smr_map_to_region(const struct fi_provider *prov, struct smr_peer *peer_buf)
{
	struct smr_region *peer;
	size_t size;
	int fd, ret = 0;

	fd = shm_open(peer_buf->peer.name, O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		FI_WARN(prov, FI_LOG_AV, "shm_open error\n");
		return -errno;
	}

	peer = mmap(NULL, sizeof(*peer), PROT_READ | PROT_WRITE,
		    MAP_SHARED, fd, 0);
	if (peer == MAP_FAILED) {
		FI_WARN(prov, FI_LOG_AV, "mmap error\n");
		ret = -errno;
		goto out;
	}

	if (!peer->pid) {
		FI_WARN(prov, FI_LOG_AV, "peer not initialized\n");
		munmap(peer, sizeof(*peer));
		ret = -FI_EAGAIN;
		goto out;
	}

	size = peer->total_size;
	munmap(peer, sizeof(*peer));

	peer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	peer_buf->region = peer;

out:
	close(fd);
	return ret;
}

void smr_map_to_endpoint(struct smr_region *region, int index)
{
	struct smr_region *peer_smr;
	struct smr_addr *local_peers, *peer_peers;
	int peer_index;

	local_peers = smr_peer_addr(region);

	strncpy(smr_peer_addr(region)[index].name,
		region->map->peers[index].peer.name, NAME_MAX - 1);
	smr_peer_addr(region)[index].name[NAME_MAX - 1] = '\0';
	if (region->map->peers[index].peer.addr == FI_ADDR_UNSPEC)
		return;

	peer_smr = smr_peer_region(region, index);
	peer_peers = smr_peer_addr(peer_smr);

	for (peer_index = 0; peer_index < SMR_MAX_PEERS; peer_index++) {
		if (!strncmp(smr_name(region),
		    peer_peers[peer_index].name, NAME_MAX))
			break;
	}
	if (peer_index != SMR_MAX_PEERS) {
		peer_peers[peer_index].addr = index;
		local_peers[index].addr = peer_index;
	}
}

void smr_unmap_from_endpoint(struct smr_region *region, int index)
{
	struct smr_region *peer_smr;
	struct smr_addr *local_peers, *peer_peers;
	int peer_index;

	local_peers = smr_peer_addr(region);

	memset(local_peers[index].name, 0, NAME_MAX);
	peer_index = region->map->peers[index].peer.addr;
	if (peer_index == FI_ADDR_UNSPEC)
		return;

	peer_smr = smr_peer_region(region, index);
	peer_peers = smr_peer_addr(peer_smr);

	peer_peers[peer_index].addr = FI_ADDR_UNSPEC;
}

void smr_exchange_all_peers(struct smr_region *region)
{
	int i;
	for (i = 0; i < SMR_MAX_PEERS; i++)
		smr_map_to_endpoint(region, i);
}

int smr_map_add(const struct fi_provider *prov, struct smr_map *map,
		const char *name, int id)
{
	int ret = 0;

	fastlock_acquire(&map->lock);
	strncpy(map->peers[id].peer.name, name, NAME_MAX);
	map->peers[id].peer.name[NAME_MAX - 1] = '\0';
	ret = smr_map_to_region(prov, &map->peers[id]);
	if (!ret)
		map->peers[id].peer.addr = id;
	fastlock_release(&map->lock);

	return ret == -ENOENT ? 0 : ret;
}

void smr_map_del(struct smr_map *map, int id)
{
	if (id >= SMR_MAX_PEERS || id < 0 ||
	    map->peers[id].peer.addr == FI_ADDR_UNSPEC)
		return;

	munmap(map->peers[id].region, map->peers[id].region->total_size);
	map->peers[id].peer.addr = FI_ADDR_UNSPEC;
}

void smr_map_free(struct smr_map *map)
{
	int i;

	for (i = 0; i < SMR_MAX_PEERS; i++)
		smr_map_del(map, i);

	free(map);
}

struct smr_region *smr_map_get(struct smr_map *map, int id)
{
	if (id < 0 || id >= SMR_MAX_PEERS)
		return NULL;

	return map->peers[id].region;
}
