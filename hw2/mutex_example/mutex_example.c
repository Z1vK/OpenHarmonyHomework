#include <stdio.h>
#include <string.h>

#include "los_mux.h"
#include "los_task.h"

static UINT32 g_testMux;
static UINT32 g_testTaskId01;
static UINT32 g_testTaskId02;

VOID Example_MutexTask1(VOID)
{
    UINT32 ret;

    printf("task1 try to get mutex, wait 10 ticks.\n");
    ret = LOS_MuxPend(g_testMux, 10);

    if (ret == LOS_OK) {
        printf("task1 get mutex g_testMux.\n");
        LOS_MuxPost(g_testMux);
        return;
    }

    if (ret == LOS_ERRNO_MUX_TIMEOUT) {
        printf("task1 timeout and try to get mutex, wait forever.\n");

        ret = LOS_MuxPend(g_testMux, LOS_WAIT_FOREVER);
        if (ret == LOS_OK) {
            printf("task1 wait forever, get mutex g_testMux.\n");
            LOS_MuxPost(g_testMux);
            LOS_MuxDelete(g_testMux);
            printf("task1 post and delete mutex g_testMux.\n");
            return;
        }
    }
}

VOID Example_MutexTask2(VOID)
{
    printf("task2 try to get mutex, wait forever.\n");
    LOS_MuxPend(g_testMux, LOS_WAIT_FOREVER);

    printf("task2 get mutex g_testMux and suspend 100 ticks.\n");
    LOS_TaskDelay(100);

    printf("task2 resumed and post the g_testMux.\n");
    LOS_MuxPost(g_testMux);
}

UINT32 Example_MutexEntry(VOID)
{
    UINT32 ret;
    TSK_INIT_PARAM_S task1;
    TSK_INIT_PARAM_S task2;

    ret = LOS_MuxCreate(&g_testMux);
    if (ret != LOS_OK) {
        printf("create mutex failed.\n");
        return LOS_NOK;
    }

    LOS_TaskLock();

    memset(&task1, 0, sizeof(TSK_INIT_PARAM_S));
    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)Example_MutexTask1;
    task1.pcName = "MutexTsk1";
    task1.uwStackSize = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    task1.usTaskPrio = 5;
    ret = LOS_TaskCreate(&g_testTaskId01, &task1);
    if (ret != LOS_OK) {
        LOS_TaskUnlock();
        printf("task1 create failed.\n");
        return LOS_NOK;
    }

    memset(&task2, 0, sizeof(TSK_INIT_PARAM_S));
    task2.pfnTaskEntry = (TSK_ENTRY_FUNC)Example_MutexTask2;
    task2.pcName = "MutexTsk2";
    task2.uwStackSize = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    task2.usTaskPrio = 4;
    ret = LOS_TaskCreate(&g_testTaskId02, &task2);
    if (ret != LOS_OK) {
        LOS_TaskUnlock();
        printf("task2 create failed.\n");
        return LOS_NOK;
    }

    LOS_TaskUnlock();
    return LOS_OK;
}
