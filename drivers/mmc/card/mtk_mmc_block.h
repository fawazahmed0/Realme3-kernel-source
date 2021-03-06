/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MT_MMC_BLOCK_H
#define _MT_MMC_BLOCK_H

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <mt-plat/mtk_blocktag.h>

#if defined(CONFIG_MMC_BLOCK_IO_LOG)

int mt_mmc_biolog_init(void);
int mt_mmc_biolog_exit(void);

void mt_bio_queue_alloc(struct task_struct *thread, struct request_queue *q);
void mt_bio_queue_free(struct task_struct *thread);

void mt_biolog_mmcqd_req_check(void);
void mt_biolog_mmcqd_req_start(struct mmc_host *host);
void mt_biolog_mmcqd_req_end(struct mmc_data *data);

void mt_biolog_cmdq_check(void);
void mt_biolog_cmdq_queue_task(unsigned int task_id, struct mmc_request *req);
void mt_biolog_cmdq_dma_start(unsigned int task_id);
void mt_biolog_cmdq_dma_end(unsigned int task_id);
void mt_biolog_cmdq_isdone_start(unsigned int task_id, struct mmc_request *req);
void mt_biolog_cmdq_isdone_end(unsigned int task_id);

#define MMC_BIOLOG_RINGBUF_MAX 120
#define MMC_BIOLOG_CONTEXTS 10       /* number of request queues */
#define MMC_BIOLOG_CONTEXT_TASKS 32  /* number concurrent tasks in cmdq */

enum {
	tsk_req_start = 0,
	tsk_dma_start,
	tsk_dma_end,
	tsk_isdone_start,
	tsk_isdone_end,
	tsk_max
};

struct mt_bio_context_task {
	int task_id;
	u32 arg;
	uint64_t t[tsk_max];
};

/* Context of Request Queue */
struct mt_bio_context {
	int id;
	int state;
	pid_t pid;
	struct request_queue *q;
	char comm[TASK_COMM_LEN+1];
	u16 qid;
	u16 q_depth;
	spinlock_t lock;
	uint64_t period_start_t;
	uint64_t period_end_t;
	uint64_t period_busy;
	uint64_t period_end_since_start_t;
	uint64_t period_end_in_window_t;
	uint64_t period_start_in_window_t;
	struct mt_bio_context_task task[MMC_BIOLOG_CONTEXT_TASKS];
	struct mtk_btag_workload workload;
	struct mtk_btag_throughput throughput;
	struct mtk_btag_pidlogger pidlog;
};

#else

#define mt_mmc_biolog_init(...)
#define mt_mmc_biolog_exit(...)

#define mt_bio_queue_alloc(...)
#define mt_bio_queue_free(...)

#define mt_biolog_mmcqd_req_check(...)
#define mt_biolog_mmcqd_req_start(...)
#define mt_biolog_mmcqd_req_end(...)

#define mt_biolog_cmdq_check(...)
#define mt_biolog_cmdq_queue_task(...)
#define mt_biolog_cmdq_dma_start(...)
#define mt_biolog_cmdq_dma_end(...)
#define mt_biolog_cmdq_isdone_start(...)
#define mt_biolog_cmdq_isdone_end(...)

#endif

#endif

