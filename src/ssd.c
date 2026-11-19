
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define SSD_SIZE 100
#define VALUE_SIZE 11

#define NAND_FILE "nand.txt"
#define RESULT_FILE "result.txt"

typedef struct {
    char data[SSD_SIZE][VALUE_SIZE];
} SSD;

/* LBA 문자열 검증: 0 ~ 99 */
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

        value = value * 10 + (str[i] - '0');
    }

    return value >= 0 && value < SSD_SIZE;
}

/* 데이터 형식 검증: 0x + 8자리 대문자 HEX */
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

/* NAND 메모리 초기화 */
void initSSD(SSD *ssd)
{
    int i;

    for (i = 0; i < SSD_SIZE; i++)
        strcpy(ssd->data[i], "0x00000000");
}

/* 기존 NAND 파일 불러오기 */
int loadNAND(SSD *ssd)
{
    FILE *fp;
    char value[64];
    char extra[64];
    int i;

    fp = fopen(NAND_FILE, "r");

    if (fp == NULL) {
        /* 파일이 없으면 초기화된 빈 SSD로 사용 */
        return 1;
    }

    for (i = 0; i < SSD_SIZE; i++) {
        if (fscanf(fp, "%63s", value) != 1 ||
            !validValue(value)) {
            fclose(fp);
            return 0;
        }

        strcpy(ssd->data[i], value);
    }

    /* 정확히 100개 데이터만 허용 */
    if (fscanf(fp, "%63s", extra) == 1) {
        fclose(fp);
        return 0;
    }

    if (ferror(fp)) {
        fclose(fp);
        return 0;
    }

    fclose(fp);
    return 1;
}

/* NAND 데이터 파일 저장 */
int saveNAND(const SSD *ssd)
{
    FILE *fp;
    int i;

    fp = fopen(NAND_FILE ".tmp", "w");

    if (fp == NULL)
        return 0;

    for (i = 0; i < SSD_SIZE; i++) {
        if (fprintf(fp, "%s\n", ssd->data[i]) < 0) {
            fclose(fp);
            remove(NAND_FILE ".tmp");
            return 0;
        }
    }

    if (fclose(fp) != 0) {
        remove(NAND_FILE ".tmp");
        return 0;
    }

    /* Windows에서 기존 파일을 교체 */
    if (remove(NAND_FILE) != 0) {
        FILE *check = fopen(NAND_FILE, "r");

        if (check != NULL) {
            fclose(check);
            remove(NAND_FILE ".tmp");
            return 0;
        }
    }

    if (rename(NAND_FILE ".tmp", NAND_FILE) != 0)
        return 0;

    return 1;
}

/* SSD Write */
int writeSSD(SSD *ssd, int lba, const char *value)
{
    strcpy(ssd->data[lba], value);

    return saveNAND(ssd);
}

/* SSD Read */
int readSSD(const SSD *ssd, int lba)
{
    FILE *fp;

    fp = fopen(RESULT_FILE, "w");

    if (fp == NULL)
        return 0;

    if (fprintf(fp, "%s\n", ssd->data[lba]) < 0) {
        fclose(fp);
        return 0;
    }

    if (fclose(fp) != 0)
        return 0;

    return 1;
}

int main(int argc, char *argv[])
{
    SSD ssd;
    int lba;

    /* 명령행 인자 검증 */
    if (argc < 3 || argc > 4) {
        printf("INVALID COMMAND\n");
        return 1;
    }

    if (!validLBA(argv[2])) {
        printf("INVALID COMMAND\n");
        return 1;
    }

    if (strcmp(argv[1], "W") == 0) {
        if (argc != 4 || !validValue(argv[3])) {
            printf("INVALID COMMAND\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "R") == 0) {
        if (argc != 3) {
            printf("INVALID COMMAND\n");
            return 1;
        }
    }
    else {
        printf("INVALID COMMAND\n");
        return 1;
    }

    lba = atoi(argv[2]);

    initSSD(&ssd);

    if (!loadNAND(&ssd)) {
        fprintf(stderr, "NAND FILE ERROR\n");
        return 1;
    }

    if (strcmp(argv[1], "W") == 0) {
        if (!writeSSD(&ssd, lba, argv[3])) {
            fprintf(stderr, "SSD WRITE FAILED\n");
            return 1;
        }
    }
    else {
        if (!readSSD(&ssd, lba)) {
            fprintf(stderr, "SSD READ FAILED\n");
            return 1;
        }
    }

    return 0;
}
