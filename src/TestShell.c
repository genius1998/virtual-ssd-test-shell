
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SSD_SIZE 100
#define INPUT_SIZE 256

/* SSD 실행파일 경로 */
#define SSD_PATH \
"C:\\Users\\SSAFY\\eclipse-workspace\\SSD_C\\Debug\\SSD_C.exe"

/* NAND 및 result.txt가 있는 폴더 */
#define SSD_DIR \
"C:\\Users\\SSAFY\\eclipse-workspace\\SSD_C"

/* LBA 검증 */
int validLBA(const char *str)
{
    int i;
    int value = 0;

    if (str == NULL || str[0] == '\0')
        return 0;

    if (strlen(str) > 2)
        return 0;

    for (i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i]))
            return 0;

        value = value * 10 + str[i] - '0';
    }

    return value < SSD_SIZE;
}

/* Write 데이터 검증 */
int validValue(const char *value)
{
    int i;

    if (value == NULL || strlen(value) != 10)
        return 0;

    if (value[0] != '0' || value[1] != 'x')
        return 0;

    for (i = 2; i < 10; i++) {
        if (!((value[i] >= '0' && value[i] <= '9') ||
              (value[i] >= 'A' && value[i] <= 'F')))
            return 0;
    }

    return 1;
}

/* SSD 실행 */
int runSSD(const char *args)
{
    char command[512];

    int n = snprintf(
        command,
        sizeof(command),
        "cd /d \"%s\" && \"%s\" %s",
        SSD_DIR, SSD_PATH, args
    );

    if (n < 0 || n >= (int)sizeof(command))
        return 0;

    return system(command) == 0;
}

/* 단일 LBA Write */
int writeSSD(int lba, const char *value)
{
    char args[64];

    snprintf(args, sizeof(args),
             "W %d %s", lba, value);

    return runSSD(args);
}

/* 단일 LBA Read */
int readSSD(int lba, char *result)
{
    char args[64];
    char path[512];
    FILE *fp;

    snprintf(args, sizeof(args), "R %d", lba);

    if (!runSSD(args))
        return 0;

    snprintf(path, sizeof(path),
             "%s\\result.txt", SSD_DIR);

    fp = fopen(path, "r");

    if (fp == NULL)
        return 0;

    if (fscanf(fp, "%10s", result) != 1) {
        fclose(fp);
        return 0;
    }

    fclose(fp);

    return validValue(result);
}

/* Full Write */
int fullWrite(const char *value)
{
    int i;

    for (i = 0; i < SSD_SIZE; i++) {
        if (!writeSSD(i, value))
            return 0;
    }

    return 1;
}

/* Full Read */
int fullRead(void)
{
    int i;
    char result[11];

    for (i = 0; i < SSD_SIZE; i++) {
        if (!readSSD(i, result))
            return 0;

        printf("LBA %d : %s\n", i, result);
    }

    return 1;
}

/* TestApp1: 전체 Write / Read 비교 */
int testApp1(void)
{
    int i;
    char result[11];
    const char *expected = "0xABCDFFFF";

    if (!fullWrite(expected))
        return 0;

    for (i = 0; i < SSD_SIZE; i++) {
        if (!readSSD(i, result))
            return 0;

        if (strcmp(result, expected) != 0) {
            printf("Mismatch at LBA %d\n", i);
            return 0;
        }
    }

    return 1;
}

/* TestApp2: 반복 Write 및 Overwrite */
int testApp2(void)
{
    int repeat;
    int lba;
    char result[11];

    const char *firstValue = "0xAAAABBBB";
    const char *finalValue = "0x12345678";

    /* LBA 0~5에 총 30회 Write */
    for (repeat = 0; repeat < 5; repeat++) {
        for (lba = 0; lba < 6; lba++) {
            if (!writeSSD(lba, firstValue))
                return 0;
        }
    }

    /* LBA 0~5 덮어쓰기 */
    for (lba = 0; lba < 6; lba++) {
        if (!writeSSD(lba, finalValue))
            return 0;
    }

    /* 최종 데이터 검증 */
    for (lba = 0; lba < 6; lba++) {
        if (!readSSD(lba, result))
            return 0;

        if (strcmp(result, finalValue) != 0) {
            printf("Mismatch at LBA %d\n", lba);
            return 0;
        }
    }

    return 1;
}

/* 도움말 */
void printHelp(void)
{
    printf("Available commands:\n");
    printf("write [LBA] [VALUE]\n");
    printf("read [LBA]\n");
    printf("fullwrite [VALUE]\n");
    printf("fullread\n");
    printf("testapp1\n");
    printf("testapp2\n");
    printf("help\n");
    printf("exit\n");
}

/* 입력을 공백 기준으로 분리 */
int parseInput(char *input, char *args[], int maxArgs)
{
    int count = 0;
    char *token = strtok(input, " \t\r\n");

    while (token != NULL) {
        if (count >= maxArgs)
            return -1;

        args[count++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    return count;
}

int main(void)
{
    char input[INPUT_SIZE];
    char *args[4];
    char result[11];
    int count;
    int lba;
    int ch;

    while (1) {
        printf(">> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        /* 너무 긴 입력은 나머지까지 제거 */
        if (strchr(input, '\n') == NULL &&
            !feof(stdin)) {
            while ((ch = getchar()) != '\n' &&
                   ch != EOF) {
            }

            printf("INVALID COMMAND\n");
            continue;
        }

        count = parseInput(input, args, 4);

        if (count <= 0) {
            if (count < 0)
                printf("INVALID COMMAND\n");
            continue;
        }

        /* write */
        if (strcmp(args[0], "write") == 0) {
            if (count != 3 ||
                !validLBA(args[1]) ||
                !validValue(args[2])) {
                printf("INVALID COMMAND\n");
                continue;
            }

            lba = atoi(args[1]);

            if (!writeSSD(lba, args[2]))
                printf("SSD WRITE FAILED\n");
        }

        /* read */
        else if (strcmp(args[0], "read") == 0) {
            if (count != 2 || !validLBA(args[1])) {
                printf("INVALID COMMAND\n");
                continue;
            }

            lba = atoi(args[1]);

            if (readSSD(lba, result))
                printf("%s\n", result);
            else
                printf("SSD READ FAILED\n");
        }

        /* fullwrite */
        else if (strcmp(args[0], "fullwrite") == 0) {
            if (count != 2 || !validValue(args[1])) {
                printf("INVALID COMMAND\n");
                continue;
            }

            if (!fullWrite(args[1]))
                printf("SSD WRITE FAILED\n");
        }

        /* fullread */
        else if (strcmp(args[0], "fullread") == 0) {
            if (count != 1) {
                printf("INVALID COMMAND\n");
                continue;
            }

            if (!fullRead())
                printf("SSD READ FAILED\n");
        }

        /* testapp1 */
        else if (strcmp(args[0], "testapp1") == 0) {
            if (count != 1) {
                printf("INVALID COMMAND\n");
                continue;
            }

            printf("TestApp1 Running...\n");

            if (testApp1())
                printf("TestApp1: PASS\n");
            else
                printf("TestApp1: FAIL\n");
        }

        /* testapp2 */
        else if (strcmp(args[0], "testapp2") == 0) {
            if (count != 1) {
                printf("INVALID COMMAND\n");
                continue;
            }

            printf("TestApp2 Running...\n");

            if (testApp2())
                printf("TestApp2: PASS\n");
            else
                printf("TestApp2: FAIL\n");
        }

        /* help */
        else if (strcmp(args[0], "help") == 0) {
            if (count != 1)
                printf("INVALID COMMAND\n");
            else
                printHelp();
        }

        /* exit */
        else if (strcmp(args[0], "exit") == 0) {
            if (count != 1) {
                printf("INVALID COMMAND\n");
                continue;
            }

            break;
        }

        else {
            printf("INVALID COMMAND\n");
        }
    }

    return 0;
}
