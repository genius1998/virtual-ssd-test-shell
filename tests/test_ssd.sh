#!/bin/sh
# 가상 SSD(src/ssd.c) 단독 검증 스크립트
# 사용법: 저장소 최상위 폴더에서  sh tests/test_ssd.sh
# 필요: gcc, POSIX sh (Linux, macOS, Windows Git Bash)
# 임시 폴더에서 실행하므로 기존 nand.txt / result.txt는 건드리지 않는다.

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

gcc -Wall -o "$WORK/ssd" "$ROOT/src/ssd.c" || { echo "BUILD FAILED"; exit 1; }
cd "$WORK" || exit 1

pass=0; fail=0
check() { # check <이름> <기대값> <실제값>
    if [ "$2" = "$3" ]; then echo "PASS  $1"; pass=$((pass+1))
    else echo "FAIL  $1 (expected: $2, actual: $3)"; fail=$((fail+1)); fi
}
run()  { ./ssd "$@" >out.txt 2>err.txt; echo $?; }
read_lba() { ./ssd R "$1" >/dev/null 2>&1 && cat result.txt; }

# 1. 처음 상태: nand.txt가 없으면 모든 LBA가 0x00000000
check "초기값 읽기 (LBA 0)"            "0x00000000" "$(read_lba 0)"
check "Read는 nand.txt를 만들지 않음"   "no"         "$( [ -f nand.txt ] && echo yes || echo no )"

# 2. 정상 쓰기/읽기
check "W 3 종료 코드"                  "0"          "$(run W 3 0xAAAABBBB)"
check "R 3 값"                         "0xAAAABBBB" "$(read_lba 3)"
check "nand.txt 100줄 생성"            "100"        "$(wc -l < nand.txt | tr -d ' ')"
check "다른 LBA(4)는 그대로"           "0x00000000" "$(read_lba 4)"

# 3. 경계값
run W 0 0x11111111 >/dev/null; run W 99 0x99999999 >/dev/null
check "LBA 0 경계"                     "0x11111111" "$(read_lba 0)"
check "LBA 99 경계"                    "0x99999999" "$(read_lba 99)"
check "LBA 두 자리 0 패딩(07) 허용"     "0"          "$(run W 07 0x07070707)"
check "R 7 값"                         "0x07070707" "$(read_lba 7)"

# 4. 같은 LBA 반복 쓰기 후 마지막 값 유지
i=0; while [ $i -lt 30 ]; do ./ssd W 5 0xAAAABBBB; i=$((i+1)); done
./ssd W 5 0x12345678
check "30회 쓰기 후 덮어쓴 값"         "0x12345678" "$(read_lba 5)"

# 5. 프로세스 재실행 후 데이터 유지 (매 명령이 새 프로세스)
check "재실행 후 LBA 3 유지"           "0xAAAABBBB" "$(read_lba 3)"

# 6. 비정상 입력: 종료 코드 1 + INVALID COMMAND, nand.txt 변화 없음
cp nand.txt before.txt
for args in "W 100 0x12345678" "R 100" "W -1 0x12345678" "R 007" "R a" \
            "W 3 0x123" "W 3 0xabcdffff" "W 3 0X12345678" "W 3 0x1234567G" \
            "X 3" "W 3" "R 3 0x12345678" "w 3 0x12345678"; do
    # shellcheck disable=SC2086
    check "거부: ssd $args" "1:INVALID COMMAND" "$(run $args):$(cat out.txt)"
done
check "비정상 입력 후 nand.txt 변화 없음" "same" "$(cmp -s before.txt nand.txt && echo same || echo diff)"

# 7. nand.txt 손상
cp before.txt nand.txt; head -n 99 before.txt > nand.txt
check "99줄 nand.txt → NAND FILE ERROR" "1:NAND FILE ERROR" "$(run R 0):$(cat err.txt)"
cp before.txt nand.txt; echo "0x00000000" >> nand.txt
check "101줄 nand.txt → NAND FILE ERROR" "1:NAND FILE ERROR" "$(run R 0):$(cat err.txt)"
cp before.txt nand.txt; sed '10s/.*/0xZZZZZZZZ/' before.txt > nand.txt
check "형식 깨진 값 → NAND FILE ERROR"   "1:NAND FILE ERROR" "$(run R 0):$(cat err.txt)"
check "손상 상태에서 Write도 거부"       "1"                 "$(run W 0 0x12345678)"
check "손상 파일은 덮어쓰지 않음"        "0xZZZZZZZZ"        "$(sed -n 10p nand.txt)"

echo "----"
echo "TOTAL: $((pass+fail))  PASS: $pass  FAIL: $fail"
[ $fail -eq 0 ]
