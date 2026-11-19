# 가상 SSD Test Shell — 상세 구현 명세 (실제 코드 기준)

> 기준 파일: `ssd.c`, `TestShell.c`. 과제 명세에서 "팀이 알아서 정하라"고 한 부분은 **[내 설계]**로 표시했어요.

---

## 1. 데이터 규격

| 항목 | 규격 | 코드 |
|---|---|---|
| LBA | 0 ~ 99, 10진수 숫자만, 최대 2자리 (`05`도 허용) | ssd.c:18-37 |
| 값 | `0x` + HEX 8자리 = 총 10글자. **[내 설계] 대문자 A~F만 허용** (`0xabcd1234`, `0X...`는 거부) | ssd.c:40-57 |
| 초기값 | 한 번도 안 쓴 LBA는 `0x00000000` | ssd.c:60-66 |

---

## 2. 가상 SSD (`ssd.exe`)

### Write 명령
- 형식: `ssd W [LBA] [값]`  예) `ssd W 3 0xAAAABBBB`
- 동작
  1. 인자 개수가 정확히 4개(`ssd W LBA 값`)인지, LBA·값 형식이 맞는지 검사
  2. 전체 100칸을 `0x00000000`으로 초기화한 뒤 nand.txt가 있으면 읽어서 덮어씀
  3. 메모리의 해당 LBA 칸만 바꿈
  4. 100칸 전체를 `nand.txt.tmp`에 쓰고 → 기존 `nand.txt` 삭제 → `.tmp`를 `nand.txt`로 이름 변경
- 화면 출력: 성공하면 없음
- 코드: ssd.c:150-155, 109-147

### Read 명령
- 형식: `ssd R [LBA]`  예) `ssd R 3`
- 동작: nand.txt를 적재한 뒤 해당 LBA 값 1개를 `result.txt`에 씀. 파일은 `"w"` 모드라 **기존 내용은 사라지고 한 줄만 남음**
- nand.txt가 없으면 모든 칸이 `0x00000000`인 상태로 읽음 (이때 nand.txt를 새로 만들지는 않음)
- 화면 출력: 없음 (결과는 result.txt에만 기록)
- 코드: ssd.c:158-176

### 종료 코드와 오류 메시지 (ssd.c:178-234)

| 상황 | 출력 | 종료 코드 |
|---|---|---|
| 성공 | 없음 | 0 |
| 인자 개수, 명령(W/R 이외), LBA, 값 형식 오류 | `INVALID COMMAND` (stdout) | 1 |
| nand.txt 형식이 깨짐 | `NAND FILE ERROR` (stderr) | 1 |
| 파일 쓰기 실패 | `SSD WRITE FAILED` / `SSD READ FAILED` (stderr) | 1 |

---

## 3. 저장 파일 형식

### nand.txt [내 설계] (ssd.c:69-147)
- 텍스트 파일, **정확히 100줄**, **N번째 줄 = LBA N-1**의 값
- 예시
  ```
  0x12345678      ← LBA 0
  0x12345678      ← LBA 1
  ...
  0x00000000      ← 안 쓴 LBA도 0으로 채워서 저장
  ```
- 처음 Write할 때 100줄 전체가 만들어짐 (안 쓴 칸은 `0x00000000`)
- 읽을 때 값이 100개보다 적거나 많거나, 형식이 하나라도 틀리면 `NAND FILE ERROR`

### result.txt (ssd.c:158-176)
- 마지막 Read 결과 **한 줄만** 저장 (예: `0x12345678`)
- Read할 때마다 덮어씀

### 파일 위치
- nand.txt와 result.txt는 ssd.exe를 실행한 **현재 작업 폴더**에 생김
- TestShell은 `cd /d <SSD 폴더>`로 이동한 뒤 실행해서 위치를 고정 (TestShell.c:15-16, 68)

---

## 4. Test Shell (`TestShell.exe`)

### 입력 공통 규칙 (TestShell.c:240-273)
- 프롬프트 `>> ` 출력 후 한 줄 입력 (최대 255글자)
- 너무 긴 입력: 남은 입력을 버리고 `INVALID COMMAND`
- 공백·탭 기준으로 단어 분리, 4개를 넘으면 `INVALID COMMAND`
- 빈 줄: 아무것도 출력하지 않고 다시 프롬프트
- 없는 명령, 인자 개수가 틀린 경우: `INVALID COMMAND`

### 명령별 동작

| 명령 | 형식 | 검증 | 동작 / 출력 | 코드 |
|---|---|---|---|---|
| write | `write 3 0xAAAABBBB` | 인자 3개, LBA, 값 | `ssd W 3 0xAAAABBBB` 실행. 성공 시 출력 없음, 실패 시 `SSD WRITE FAILED` | TestShell.c:276-288, 79-87 |
| read | `read 3` | 인자 2개, LBA | `ssd R 3` 실행 → 성공하면 result.txt를 읽어 값 출력 (`0xAAAABBBB`). 실패 시 `SSD READ FAILED` | TestShell.c:291-303, 90-117 |
| fullwrite | `fullwrite 0xABCDFFFF` | 인자 2개, 값 | LBA 0~99에 차례로 write (ssd.exe 100번 실행). 중간 실패 시 멈추고 `SSD WRITE FAILED` | TestShell.c:306-314, 120-130 |
| fullread | `fullread` | 인자 1개 | LBA 0~99를 읽어 `LBA 0 : 0x...` 형식으로 100줄 출력. 중간 실패 시 거기까지만 출력 후 `SSD READ FAILED` | TestShell.c:317-325, 133-146 |
| testapp1 | `testapp1` | 인자 1개 | 아래 참고 | TestShell.c:149-169 |
| testapp2 | `testapp2` | 인자 1개 | 아래 참고 | TestShell.c:172-207 |
| help | `help` | 인자 1개 | 사용 가능한 명령 8개 목록 출력 | TestShell.c:210-221 |
| exit | `exit` | 인자 1개 | 반복문 종료, 프로그램 끝 (입력 끝 EOF도 종료) | TestShell.c:366-373, 253-254 |

### Read 값 받아오는 방식 (TestShell.c:90-117)
1. `ssd R n` 실행 → 종료 코드가 0이 아니면 바로 실패 처리 (이전 result.txt를 읽지 않음)
2. result.txt를 열어 최대 10글자 읽음
3. 읽은 값이 `0x` + HEX 8자리인지 **한 번 더 검증**한 뒤 출력

---

## 5. 테스트 시나리오

### testapp1 (TestShell.c:149-169)
1. `fullwrite 0xABCDFFFF` (LBA 0~99)
2. LBA 0~99를 하나씩 읽어 `0xABCDFFFF`와 비교
3. 출력: `TestApp1 Running...` → `TestApp1: PASS` / 불일치 시 `Mismatch at LBA n` + `TestApp1: FAIL`

### testapp2 (TestShell.c:172-207)
1. LBA 0~5에 `0xAAAABBBB` 쓰기를 5번 반복 (총 30회)
2. LBA 0~5에 `0x12345678` 덮어쓰기
3. LBA 0~5를 읽어 `0x12345678`과 비교
4. 출력: `TestApp2 Running...` → `TestApp2: PASS` / `FAIL`

### 실행 결과 (ssd_화면.jpg)
- `write 3 0xAAAABBBB` → `read 3` = `0xAAAABBBB`
- `write 97 0x12345678` → `read 97` = `0x12345678`
- `fullwrite 0xABCDFFFF` → `read 3` = `0xABCDFFFF`
- testapp1 PASS, testapp2 PASS
- 이후 `read 0` = `0x12345678` (testapp2가 덮어쓴 범위), `read 6` = `0xABCDFFFF` (범위 밖이라 그대로)

---

## 과제 명세와 다른 점 / 직접 정한 점

- 명세 범위는 `0x00000000 ~ 0xFFFFFFFF`인데, 코드는 **대문자 HEX만** 받음 → 의도한 것이면 README에 명시
- nand.txt 형식(줄 = LBA, 100줄 고정)과 임시 파일 교체 방식은 명세에 없고 직접 설계한 부분
