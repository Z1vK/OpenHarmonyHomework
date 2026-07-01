/*
 * simple_editor.c —— OpenHarmony (LiteOS-M) 简易 shell 文本编辑器
 * 命令: text <filename>    在 /littlefs/ 下创建/编辑文件
 *       text -d <filename> 删除文件
 * 环境: OpenHarmony v3.2-release / qemu_mini_system_demo (mps2-an386)
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "ohos_init.h"
#include "shell.h"
#include "los_task.h"

#define BUFFER_SIZE 2048
#define BASE_PATH   "/littlefs/"

/* [1] 全局状态 */
static int  g_file_fd = -1;
static char g_text_buffer[BUFFER_SIZE];
static int  g_write_pos = 0;
static char g_file_path[256];

/* 板级串口收发(本板 stdin/stdout 未接串口, 交互需直接读写 UART) */
extern int UartGetc(void);
extern int UartPutc(int c, void *file);

static int EditorGetChar(void)
{
    int c;
    while ((c = UartGetc()) == 0) {
        LOS_TaskDelay(1);
    }
    return c;
}

static void EditorEcho(char ch)
{
    (void)UartPutc((int)ch, 0);
}

/* [6] 使用说明 */
void EditorShowHelp(void)
{
    printf("--- Text Editor Help ---\n");
    printf("Usage: text <filename>    -> create/edit\n");
    printf("       text -d <filename> -> delete\n");
    printf("Path : %s\n", BASE_PATH);
    printf("Keys : [Enter] save&exit  [Backspace] delete  [Ctrl+C] clear line\n");
}

/* [2] 删除文件 */
static void DeleteFile(const char *name)
{
    snprintf(g_file_path, sizeof(g_file_path), "%s%s", BASE_PATH, name);
    if (unlink(g_file_path) == 0) {
        printf("[SUCCESS] File %s deleted.\n", name);
    } else {
        printf("[ERROR] delete %s failed, errno=%d.\n", name, errno);
    }
}

/* [2][3][4] 打开+自动创建, 读出旧内容并打印, 指针移到末尾, 再以写+截断重开 */
static int OpenAndLoadFile(const char *name)
{
    snprintf(g_file_path, sizeof(g_file_path), "%s%s", BASE_PATH, name);
    memset(g_text_buffer, 0, BUFFER_SIZE);
    g_write_pos = 0;

    g_file_fd = open(g_file_path, O_RDWR | O_CREAT, 0666);
    if (g_file_fd < 0) {
        printf("[ERROR] open %s failed, errno=%d.\n", g_file_path, errno);
        return -1;
    }

    int len = read(g_file_fd, g_text_buffer, BUFFER_SIZE - 1);
    if (len > 0) {
        g_text_buffer[len] = '\0';
        printf("--- File Content ---\n%s\n--------------------\n", g_text_buffer);
        g_write_pos = len;
    } else {
        printf("[INFO] New / empty file.\n");
    }
    close(g_file_fd);

    /* 旧内容已读入缓冲, 重开为写+截断, 保存时整段覆盖写回 */
    g_file_fd = open(g_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (g_file_fd < 0) {
        printf("[ERROR] reopen %s failed, errno=%d.\n", g_file_path, errno);
        return -1;
    }
    return 0;
}

/* [3] 保存: lseek 到头, 整段写入, fsync 刷盘 */
static void SaveFile(void)
{
    if (g_file_fd < 0) {
        return;
    }
    lseek(g_file_fd, 0, SEEK_SET);
    int n = write(g_file_fd, g_text_buffer, g_write_pos);
    if (n != g_write_pos) {
        printf("[ERROR] write failed, want=%d got=%d.\n", g_write_pos, n);
    }
    fsync(g_file_fd);
}

/* [5] 输入循环: 退格 / 回车保存退出 / Ctrl+C 清空当前行 */
static void StartInputLoop(void)
{
    int base = g_write_pos;   /* 本次输入起点(已加载内容之后) */

    printf("Start typing ([Enter] save, [Backspace] delete, [Ctrl+C] clear):\n> ");
    fflush(stdout);

    while (1) {
        char ch = (char)EditorGetChar();

        if (ch == '\r' || ch == '\n') {
            printf("\nSaving to disk...\n");
            SaveFile();
            close(g_file_fd);
            g_file_fd = -1;
            printf("[SUCCESS] file saved and closed.\n");
            return;
        } else if (ch == 0x03) {                 /* Ctrl+C: 清空本次输入 */
            while (g_write_pos > base) {
                g_write_pos--;
                EditorEcho('\b'); EditorEcho(' '); EditorEcho('\b');
            }
            g_text_buffer[g_write_pos] = '\0';
        } else if (ch == '\b' || ch == 0x7F) {   /* 退格 */
            if (g_write_pos > base) {
                g_write_pos--;
                g_text_buffer[g_write_pos] = '\0';
                EditorEcho('\b'); EditorEcho(' '); EditorEcho('\b');
            }
        } else if (g_write_pos < BUFFER_SIZE - 1) {
            g_text_buffer[g_write_pos++] = ch;
            EditorEcho(ch);
        } else {
            printf("\n[WARN] buffer full, auto save.\n");
            SaveFile();
            close(g_file_fd);
            g_file_fd = -1;
            return;
        }
    }
}

/* [7] 主命令处理 */
int CmdTextEditor(int argc, const char **argv)
{
    if (argc < 2) {
        EditorShowHelp();
        return 0;
    }
    if (strcmp(argv[1], "-d") == 0) {
        if (argc >= 3) {
            DeleteFile(argv[2]);
        } else {
            printf("Usage: text -d <filename>\n");
        }
        return 0;
    }
    if (OpenAndLoadFile(argv[1]) == 0) {
        StartInputLoop();
    }
    return 0;
}

/* [8] 注册: 先 OsShellInit 建好命令表再 osCmdReg, 避免过早注册失败 */
extern UINT32 OsShellInit(VOID);

static volatile int g_regStarted = 0;

static void TextEditorRegEntry(void)
{
    (void)OsShellInit();
    if (osCmdReg(CMD_TYPE_STD, "text", 0, (CMD_CBK_FUNC)CmdTextEditor) == LOS_OK) {
        printf("[text] 'text' command registered.\n");
    } else {
        printf("[text] register 'text' failed.\n");
    }
}

void TextEditorInit(void)
{
    if (g_regStarted) {          /* main.c 调用与 SYS_RUN 只注册一次 */
        return;
    }
    g_regStarted = 1;

    UINT32 taskId;
    TSK_INIT_PARAM_S param = {0};
    param.pfnTaskEntry = (TSK_ENTRY_FUNC)TextEditorRegEntry;
    param.uwStackSize  = 0x1000;
    param.pcName       = "TextEditorReg";
    param.usTaskPrio   = 20;
    (void)LOS_TaskCreate(&taskId, &param);
}

SYS_RUN(TextEditorInit);