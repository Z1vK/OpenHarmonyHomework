#include <stdio.h>
#include <string.h>

#include "los_queue.h"
#include "los_task.h"

#define BUFFER_LEN 50

static UINT32 g_queue;
static UINT32 g_sendTaskId;
static UINT32 g_recvTaskId;

VOID SendEntry(VOID)
{
    UINT32 ret = 0;
    CHAR abuf[] = "test message";
    UINT32 len = sizeof(abuf);

    ret = LOS_QueueWriteCopy(g_queue, abuf, len, 0);
    if (ret != LOS_OK) {
        printf("Failed to send the message. Error: %#x\n", ret);
    }
}

VOID RecvEntry(VOID)
{
    UINT32 ret = 0;
    CHAR readBuf[BUFFER_LEN] = {0};
    UINT32 readLen = BUFFER_LEN;

    ret = LOS_QueueReadCopy(g_queue, readBuf, &readLen, 0);
    if (ret != LOS_OK) {
        printf("Failed to receive the message. Error: %#x\n", ret);
        return;
    }

    printf("recv message: %s\n", readBuf);

    ret = LOS_QueueDelete(g_queue);
    if (ret != LOS_OK) {
        printf("Failed to delete the queue. Error: %#x\n", ret);
        return;
    }

    printf("delete the queue success!\n");
}

UINT32 Example_QueueEntry(VOID)
{
    UINT32 ret = 0;
    TSK_INIT_PARAM_S initParam = {0};

    printf("start test example\n");

    initParam.pfnTaskEntry = (TSK_ENTRY_FUNC)SendEntry;
    initParam.usTaskPrio = 9;
    initParam.uwStackSize = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    initParam.pcName = "SendQueue";

    LOS_TaskLock();
    ret = LOS_TaskCreate(&g_sendTaskId, &initParam);
    if (ret != LOS_OK) {
        LOS_TaskUnlock();
        printf("create task1 failed, error: %#x\n", ret);
        return ret;
    }

    initParam.pcName = "RecvQueue";
    initParam.pfnTaskEntry = (TSK_ENTRY_FUNC)RecvEntry;
    ret = LOS_TaskCreate(&g_recvTaskId, &initParam);
    if (ret != LOS_OK) {
        LOS_TaskUnlock();
        printf("create task2 failed, error: %#x\n", ret);
        return ret;
    }

    ret = LOS_QueueCreate("queue", 5, &g_queue, 0, BUFFER_LEN);
    if (ret != LOS_OK) {
        LOS_TaskUnlock();
        printf("create queue failure, error: %#x\n", ret);
        return ret;
    }

    printf("create the queue success!\n");
    LOS_TaskUnlock();
    return ret;
}
