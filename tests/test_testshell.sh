#!/bin/sh
# Test Shell(src/TestShell.c) 시나리오 검증 스크립트 (Linux / macOS용)
# 사용법: 저장소 최상위 폴더에서  sh tests/test_testshell.sh
#
# TestShell.c는 Windows 절대 경로와 "cd /d"를 쓰기 때문에 그대로는 Linux에서 돌지 않는다.
# 원본은 수정하지 않고, 임시 폴더에 경로 4곳만 바꾼 사본을 만들어 빌드한 뒤
# 표준 입력으로 명령을 넣어 출력과 기대값을 비교한다.

ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

gcc -Wall -o "$WORK/ssd" "$ROOT/src/ssd.c" || { echo "BUILD FAILED (ssd)"; exit 1; }
sed -e "s|\"C:.*SSD_C.exe\"|\"$WORK/ssd\"|" \
    -e "s|\"C:.*SSD_C\"|\"$WORK\"|" \
    -e 's|cd /d |cd |' \
    -e 's|"%s\\\\result.txt"|"%s/result.txt"|' \
    "$ROOT/src/TestShell.c" > "$WORK/TestShell_linux.c"
gcc -Wall -o "$WORK/TestShell" "$WORK/TestShell_linux.c" || { echo "BUILD FAILED (TestShell)"; exit 1; }

LONG254=$(printf '%254s' '' | tr ' ' 'a')   # 한 줄 254자: 버퍼에 들어감 → 없는 명령
LONG255=$(printf '%255s' '' | tr ' ' 'a')   # 한 줄 255자: 버퍼 초과 → 길이 초과로 거부

cat > "$WORK/input.txt" <<IN
write 3 0xAAAABBBB
read 3
fullwrite 0xABCDFFFF
read 0
read 50
read 99
testapp1
testapp2
read 0
read 5
read 6
read 100
write 3 0x123
write 3 0xabcdffff
hello
write 3
write 1 2 3 4
help extra

$LONG254
$LONG255
read 3
exit
IN

cat > "$WORK/expected.txt" <<'EX'
0xAAAABBBB
0xABCDFFFF
0xABCDFFFF
0xABCDFFFF
TestApp1 Running...
TestApp1: PASS
TestApp2 Running...
TestApp2: PASS
0x12345678
0x12345678
0xABCDFFFF
INVALID COMMAND
INVALID COMMAND
INVALID COMMAND
INVALID COMMAND
INVALID COMMAND
INVALID COMMAND
INVALID COMMAND
INVALID COMMAND
INVALID COMMAND
0x12345678
EX

(cd "$WORK" && ./TestShell < input.txt) | sed 's/^\(>> \)*//' | grep -v '^$' > "$WORK/actual.txt"

if diff "$WORK/expected.txt" "$WORK/actual.txt"; then
    echo "PASS  Test Shell 시나리오 $(wc -l < "$WORK/expected.txt" | tr -d ' ')줄 출력 일치"
else
    echo "FAIL  위 diff 참고 (< 기대값, > 실제값)"; exit 1
fi
