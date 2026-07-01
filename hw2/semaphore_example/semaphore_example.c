#include <stdio.h>
#include <string.h>

#include "los_sem.h"
#include "los_task.h"

static UINT32 g_semId;
static UINT32 g_semTaskId01;
static UINT32 g_semTaskId02;

VOID ExampleSemTask1(VOID)
{
    UINT32 ret;

    printf("ExampleSemTask1 try get sem g_semId, timeout 10 ticks.\n");

    ret = LOS_SemPend(g_semId, 10);
    if (ret == LOS_OK) {
        LOS_SemPost(g_semId);
        return;
    }

    if (ret == LOS_ERRNO_SEM_TIMEOUT) {
        printf("ExampleSemTask1 timeout and try get sem g_semId wait forever.\n");

        ret = LOS_SemPend(g_semId, LOS_WAIT_FOREVER);
        printf("ExampleSemTask1 wait_forever and get sem g_semId.\n");
        if (ret == LOS_OK) {
            LOS_SemPost(g_semId);
            return;
        }
    }
}

VOID ExampleSemTask2(VOID)
{
    UINT32 ret;

    printf("ExampleSemTask2 try get sem g_semId wait forever.\n");

    ret = LOS_SemPend(g_semId, LOS_WAIT_FOREVER);
    if (ret == LOS_OK) {
        printf("ExampleSemTask2 get sem g_semId and then delay 20 ticks.\n");
    }

    LOS_TaskDelay(20);

    printf("ExampleSemTask2 post sem g_semId.\n");
    LOS_SemPost(g_semId);
}

UINT32 Example_SemEntry(VOID)
{
    UINT32 ret;
    TSK_INIT_PARAM_S task1;
    TSK_INIT_PARAM_S task2;

    ret = LOS_SemCreate(1, &g_semId);
    if (ret != LOS_OK) {
        printf("create sem failed.\n");
        return LOS_NOK;
    }

    LOS_TaskLock();

    memset(&task1, 0, sizeof(TSK_INIT_PARAM_S));
    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)ExampleSemTask1;
    task1.pcName = "ExampleSemTask1";
    task1.uwStackSize = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    task1.usTaskPrio = 5;
    ret = LOS_TaskCreate(&g_semTaskId01, &task1);
    if (ret != LOS_OK) {
        LOS_TaskUnlock();
        printf("create ExampleSemTask1 failed.\n");
        return LOS_NOK;
    }

    memset(&task2, 0, sizeof(TSK_INIT_PARAM_S));
    task2.pfnTaskEntry = (TSK_ENTRY_FUNC)ExampleSemTask2;
    task2.pcName = "ExampleSemTask2";
    task2.uwStackSize = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    task2.usTaskPrio = 4;
    ret = LOS_TaskCreate(&g_semTaskId02, &task2);
    if (ret != LOS_OK) {
        LOS_TaskUnlock();
        printf("create ExampleSemTask2 failed.\n");
        return LOS_NOK;
    }

    LOS_TaskUnlock();
    LOS_TaskDelay(400);

    ret = LOS_SemDelete(g_semId);
    if (ret != LOS_OK) {
        printf("delete sem failed.\n");
        return LOS_NOK;
    }

    return LOS_OK;
}
