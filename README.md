# Level Devil — 공통 기반 + 첫 번째 방 (1-1)

C++17 / SFML 3.1.0으로 만든 Windows 게임입니다. 원작 첫 방의 화면과 함정 구성을 참고해 직접 구현했습니다.

## 실행

- `Play.cmd`를 더블클릭하거나 `bin\Release\LevelDevil.exe`를 실행하세요.
- 처음부터 게임 화면이 열립니다. 오른쪽 문에 닿으면 캐릭터가 문 안으로 들어간 뒤 1-1이 클리어됩니다.
- 사망하면 약 0.38초 뒤 자동 재시작합니다. 목숨 제한은 없습니다.
- 클리어 후 `Enter` 또는 `PLAY AGAIN` 버튼으로 다시 시작합니다.

| 키 | 동작 |
| --- | --- |
| A / D 또는 ← / → | 좌우 이동 |
| Space / W / ↑ | 점프 |
| R | 현재 방 재시작 |
| Esc / P | 일시정지 / 계속 |
| 일시정지 상태에서 Q | 종료 |

왼쪽 위의 일시정지 아이콘과 다시 시작 아이콘도 클릭할 수 있습니다. 다른 창으로 전환하면 자동으로 일시정지합니다. 창 크기를 바꿔도 화면 비율과 충돌 좌표를 유지합니다.

## 구현 범위

현재 플레이 가능한 맵은 **1-1 하나**입니다. 공통 플레이어 물리, 스테이지 인터페이스, 순차 전환 관리자를 먼저 분리해 두었으며 이후 `Stage2`부터 같은 계약으로 연결할 수 있습니다.

- 원작 1-1 화면 비율에 맞춘 황토색 바깥, 옅은 주황색 통로, 작은 검은 캐릭터, 회색 출구
- 원작처럼 왼쪽 위에 배치한 일시정지·다시 시작 아이콘과 상단 진행 칸
- 공식 영상의 작은 머리·연결된 몸통·보폭을 참고한 2px 격자 캐릭터, 좌우 걷기와 공중 자세
- 접근하면 출구 직전의 바닥 한 칸이 위에서 아래로 빠르게 벌어지며 구멍이 열리는 함정
- 문에 닿으면 자동으로 안쪽까지 걸어 들어가며 작아지고 사라지는 출구 연출
- 중력, 좌우 이동, 점프, 벽/천장/바닥/구멍 옆면 충돌
- 사망 횟수 누적, 자동 재시작, 클리어 표시, 일시정지
- 외부 이미지·폰트 없이 도형과 비트맵 글꼴로 표현
- 효과음과 SFML Audio 의존성은 프로젝트 범위에서 제외

원작 소스와 에셋을 복사한 버전은 아닙니다. 배치와 색상은 참고 화면에 맞췄고, 이동 속도·점프 높이·함정 발동 거리·재시작 시간은 직접 조정한 값입니다. 원작과 수치까지 동일하다는 의미는 아닙니다.

캐릭터는 공식 Steam 예고편의 연속 90프레임과 1-1 스크린샷을 픽셀 격자로 재구성했습니다. 달리기는 두 발이 뜨는 자세 → 뒤쪽 발 → 발 교차 → 앞쪽 발 순서로 반복하며, 시각적인 발 높이가 바닥에서 6px → 2px → 0px → 0px로 바뀝니다. 대기 자세는 달리기 주기에 포함하지 않습니다. 반복 주기는 영상에서 측정한 약 280ms이며, 원본 내부 애니메이션 상수를 그대로 가져온 것은 아닙니다.

## Visual Studio에서 수정 / 빌드

1. 저장소를 처음 받은 경우 `Build.cmd`를 실행해 SFML 의존성을 준비하고 빌드합니다.
2. `LevelDevil.slnx`를 Visual Studio 2026에서 엽니다.
3. `LevelDevil`을 시작 프로젝트로 선택합니다.
4. `Debug | x64` 또는 `Release | x64`를 선택하고 실행합니다.

Visual Studio의 **C++를 사용한 데스크톱 개발**, v145 도구 집합, Windows SDK가 필요합니다. 빌드 스크립트는 SFML을 프로젝트 내부 `external\SFML-3.1.0`에 준비하며 각 구성에 맞는 라이브러리/DLL을 자동으로 연결하고 복사합니다. SFML 패키지와 빌드 산출물은 Git 저장소에서 제외합니다.

명령줄 빌드:

```powershell
.\Build.ps1 -Configuration Debug
.\Build.ps1 -Configuration Release
```

`Build.cmd` 더블클릭으로도 Release 빌드와 물리 검증을 실행합니다. 의존성 폴더가 없으면 공식 SFML 패키지를 받아 SHA-256 확인 후 복구합니다. 버전/출처는 `external\SFML-SOURCE.md`에 있습니다.

## 파일

- `src\Common.hpp`: 화면 크기, 플레이어 크기, 속도, 중력, 점프력, 입력/상태 자료형
- `src\IStage.hpp`: 모든 `StageX`가 구현하는 공통 스테이지 계약
- `src\GameModel.hpp/.cpp`: 공통 플레이어 물리, 충돌, 사망/재시작, 문 진입과 클리어 상태
- `src\RenderCommon.hpp/.cpp`: 공통 색상, 글자, 문, 캐릭터와 걷기 애니메이션
- `src\StageManager.hpp/.cpp`: 스테이지 등록과 순차 전환, 전체 재시작
- `src\Stage1.hpp/.cpp`: 1-1 배치, 구멍 함정 상태, 문 판정과 맵 그리기
- `src\main.cpp`: SFML 창, 키/마우스 입력, 고정 시간 간격 실행, HUD
- `STAGE_GUIDE.md`: 조원별 `StageX` 작성 범위와 연결 방법
- `tests\GameModelTests.cpp`: 공통 물리와 Stage1 낙사/클리어 경로 검증
- `artifacts\verification`: 게임과 동일한 렌더러로 만든 시작/함정/점프/클리어 화면 및 결과
- `artifacts\verification\04-door-entering.png`: 문에 닿은 뒤 자동으로 들어가는 중간 상태
- `artifacts\verification\06-character-poses.png`: 좌우 대기·걷기·점프의 실제 크기와 5배 확대 비교
- `artifacts\walk-verification\walk-motion.mp4`: 실제 렌더러의 60fps 좌우 보행·정지·점프·착지 영상
- `artifacts\walk-verification\walk-motion.gif`: 같은 동작을 확인하는 애니메이션 미리보기
- `artifacts\walk-verification\reference-cycle.json`: 공식 영상의 보행 순서·지속 시간·높이 측정 근거

위 `artifacts` 경로의 자료는 로컬 검증 산출물이며 Git 저장소에 포함하지 않습니다. 게임 화면 검증은 아래 명령으로 다시 생성할 수 있습니다.

게임 화면 검증 재실행:

```powershell
$game = Start-Process -FilePath .\bin\Release\LevelDevil.exe `
  -ArgumentList '--verify artifacts/verification' -Wait -PassThru -WindowStyle Hidden
$game.ExitCode
```

소스를 수정한 뒤에는 빌드하고 `Play.cmd`로 실행하세요. `Play.cmd`는 실행 파일이 없을 때만 자동 빌드합니다.

`LevelDevil.exe --verify-motion artifacts/walk-verification/frames`는 실제 입력과 렌더러로 4초 분량의 60fps PNG 시퀀스와 상태 기록을 생성합니다. 영상 인코딩은 검증 자료 제작에만 사용하며 게임 실행에는 필요하지 않습니다.

## 참고

- 사용자 제공 [나무위키 문서](https://namu.wiki/w/Level%20Devil)는 이 환경에서 열리지 않아 아래의 실제 화면과 1-1 플레이 영상을 추가 확인했습니다.
- [공식 배포처 Poki — Level Devil](https://poki.com/en/g/level-devil)
- [공식 Steam 페이지의 스크린샷과 예고편 — 캐릭터 외형 참고](https://store.steampowered.com/app/3242750/Level_Devil/)
- [첫 방 1-1 참고 화면](https://zotuf.com/answer/level-devil/1/1-1.jpg)
- [방별 플레이 영상](https://zotuf.com/level-devil-walkthrough/)
- [확인한 1-1 영상](https://rutube.ru/play/embed/115f472e01dc626242d86b4bad662b27/?p=K7Zh9dnmy3VDXZXGl0ozuQ)
- [SFML 3.1.0](https://www.sfml-dev.org/download/sfml/3.1.0/)

Level Devil 원작은 Unept의 게임이며, 이 프로젝트는 학습용으로 독립 구현한 첫 방입니다.
