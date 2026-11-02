# Level Devil — Stage1~Stage6 + 보너스 FinalStage

C++17 / SFML 3.1.0으로 만든 Windows 게임입니다. 원작의 바닥·가시·코인·Push·다단 함정·Saws·최종 구간을 참고해 팀별 스테이지로 구현했습니다.

## 발표자료와 시연영상

게임 코드는 `src/`, 자동 검증은 `tests/`에서 관리하며, 발표자료와 시연영상은 `docs/` 아래에서 구분합니다.

- [5조 발표자료](<docs/presentation/1.5차 팀 프로젝트 5조.pptx>)
- [발표용 프로젝트 구조 설명](docs/presentation/PPT_PROJECT_STRUCTURE.md)
- [시연영상](<docs/demo/시연 영상.mp4>)

## 실행

- `Play.cmd`를 더블클릭하거나 `bin\Release\LevelDevil.exe`를 실행하세요.
- 처음부터 게임 화면이 열립니다. `Stage1 → Stage2 → Stage3 → Stage4 → Stage5 → Stage6 → FinalStage` 순서로 이어집니다.
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

현재 플레이 가능한 맵은 팀의 `Stage1`~`Stage6`와 보너스 `FinalStage`까지 총 7개입니다. `StageManager`가 이 순서대로 연결하며 공통 플레이어 물리, 스테이지 인터페이스와 각 방의 기믹은 분리되어 있습니다.

- 원작 1-1 화면 비율에 맞춘 황토색 바깥, 옅은 주황색 통로, 작은 검은 캐릭터, 회색 출구
- 원작처럼 왼쪽 위에 배치한 일시정지·다시 시작 아이콘과 상단 진행 칸
- 공식 영상의 작은 머리·연결된 몸통·보폭을 참고한 2px 격자 캐릭터, 좌우 걷기와 공중 자세
- 접근하면 출구 직전의 바닥 한 칸이 위에서 아래로 빠르게 벌어지며 구멍이 열리는 함정
- 문에 닿으면 자동으로 안쪽까지 걸어 들어가며 작아지고 사라지는 출구 연출
- 2번 방의 이동 가시와 연속 구멍, 3번 방의 코인 연동 바닥·벽·천장 함정
- 4번 방의 Push 블록과 문 앞 구멍, 5번 방의 3페이즈 바닥·계단·기둥·가짜 문
- 6번 방의 시작 절벽 추격 톱날, 세 칸 임시 발판 붕괴, 높이가 다른 세 대형 톱날의 단방향 윗면을 이용한 상승 코스
- 보너스 최종 방의 위층 추격 벽·낙하 바닥·시간제 Push 벽·가시, 교차 벽 톱날 낙하, 윗부분만 노출되는 상승 톱날·구멍·발사체
- 보너스 최종 문 진입 뒤 `YOU ESCAPED`에서 데빌의 눈과 `7 / 7` 결과 화면으로 이어지는 엔딩
- 중력, 좌우 이동, 점프, 벽/천장/바닥/구멍 옆면 충돌
- 사망 횟수 누적, 자동 재시작, 클리어 표시, 일시정지
- 외부 이미지·폰트 없이 도형과 비트맵 글꼴로 표현
- 효과음과 SFML Audio 의존성은 프로젝트 범위에서 제외

원작 소스와 에셋을 복사한 버전은 아닙니다. 배치와 색상은 참고 화면에 맞췄고, 이동 속도·점프 높이·함정 발동 거리·재시작 시간은 직접 조정한 값입니다. 원작과 수치까지 동일하다는 의미는 아닙니다.

캐릭터는 공식 Steam 예고편의 연속 90프레임과 1-1 스크린샷을 픽셀 격자로 재구성했습니다. 달리기는 두 발이 뜨는 자세 → 뒤쪽 발 → 발 교차 → 앞쪽 발 순서로 반복하며, 시각적인 발 높이가 바닥에서 6px → 2px → 0px → 0px로 바뀝니다. 대기 자세는 달리기 주기에 포함하지 않습니다. 반복 주기는 영상에서 측정한 약 280ms이며, 원본 내부 애니메이션 상수를 그대로 가져온 것은 아닙니다.

사용자 제공 Push 플레이 영상의 30fps 프레임에서 이동량과 점프 궤적을 비교해 공통 좌우 속도를 192, 중력을 2048, 점프 속도를 472로 조정했습니다. 방향 입력은 즉시 반응하고 공중에서도 같은 좌우 속도를 유지합니다. 짧고 낮은 점프에 맞춰 톱날 판정과 발판 간격도 함께 조정했습니다.

## Visual Studio에서 수정 / 빌드

1. 저장소를 처음 받은 경우 `Build.cmd`를 실행해 SFML 의존성을 준비하고 빌드합니다.
2. `LevelDevil.slnx`를 Visual Studio 2026에서 엽니다.
3. `LevelDevil`을 시작 프로젝트로 선택합니다.
4. `Debug | x64` 또는 `Release | x64`를 선택하고 실행합니다.

Visual Studio의 **C++를 사용한 데스크톱 개발**, v143 도구 집합, Windows SDK가 필요합니다. 빌드 스크립트는 SFML을 프로젝트 내부 `external\SFML-3.1.0`에 준비하며 각 구성에 맞는 라이브러리/DLL을 자동으로 연결하고 복사합니다. SFML 패키지와 빌드 산출물은 Git 저장소에서 제외합니다.

명령줄 빌드:

```powershell
.\Build.ps1 -Configuration Debug
.\Build.ps1 -Configuration Release
```

`Build.cmd` 더블클릭으로도 Release 빌드와 물리 검증을 실행합니다. 의존성 폴더가 없으면 공식 SFML 패키지를 받아 SHA-256 확인 후 복구합니다. 버전/출처는 `external\SFML-SOURCE.md`에 있습니다.

## 현재 검증 상태

2026-11-02 로컬에서 상위 폴더의 공통 빌드 설정을 제외하고 확인한 결과입니다. 게임 동작과 기존 검증 코드는 이번 업로드에서 수정하지 않았습니다.

| 구성 | 컴파일 | 기존 자동검증 |
| --- | --- | --- |
| Debug / x64 | 성공 | 627개 검사 중 18개 실패 |
| Release / x64 | 성공 | 627개 검사 중 18개 실패 |

실패 항목은 스테이지 표시명 비교 4개, Stage5 통과 경로 5개, FinalStage 함정 순서 1개와 연속 통과 경로 8개입니다. 검증 코드와 현재 게임 동작 사이의 차이인지 게임 결함인지 추가 확인이 필요합니다. Stage5에는 상수 조건식에 대한 C4127 컴파일 경고도 있습니다.

현재 `Build.cmd`와 `Build.ps1`의 기본 실행은 자동검증 실패로 종료합니다. 컴파일만 확인하려면 `./Build.ps1 -Configuration Release -SkipTests`를 실행하고, 자동검증은 `bin/Release/GameModelTests.exe`로 별도 실행하세요.

## 파일

- `src\Common.hpp`: 화면 크기, 플레이어 크기, 속도, 중력, 점프력, 입력/상태 자료형
- `src\IStage.hpp`: 모든 `StageX`가 구현하는 공통 스테이지 계약
- `src\GameModel.hpp/.cpp`: 공통 플레이어 물리, 충돌, 사망/재시작, 문 진입과 클리어 상태
- `src\RenderCommon.hpp/.cpp`: 공통 색상, 글자, 문, 캐릭터와 걷기 애니메이션
- `src\StageManager.hpp/.cpp`: 스테이지 등록과 순차 전환, 전체 재시작
- `src\Stage1.hpp/.cpp`: 1-1 배치, 구멍 함정 상태, 문 판정과 맵 그리기
- `src\Stage2.hpp/.cpp`: 이동 가시, 연속 구멍과 역방향 진행 구간
- `src\Stage3*.hpp/.cpp`: 코인 트리거, 사라지는 바닥, 이동 벽과 낙하 천장
- `src\Stage4.hpp/.cpp`: 플레이어를 미는 벽, 상승·낙하 블록과 문 앞 구멍
- `src\Stage5.hpp/.cpp`: 다단 바닥 함정, 상승 계단·기둥, 이동 문과 가짜 문
- `src\Stage6.hpp/.cpp`: 톱날 방의 추격 톱날·붕괴 발판·3단 상승 톱날 상태
- `src\FinalStage.hpp/.cpp`: 2층 보너스 최종 방의 낙하 통로, 시간제 Push 벽과 혼합 함정 상태
- `src\main.cpp`: SFML 창, 키/마우스 입력, 고정 시간 간격 실행, HUD
- `STAGE_GUIDE.md`: 조원별 `StageX` 작성 범위와 연결 방법
- `tests\GameModelTests.cpp`: 7개 스테이지 등록과 공통 물리, Stage1~Stage6·FinalStage의 상태 및 사망 0회 통과 경로 검증
- `artifacts\verification`: 게임과 동일한 렌더러로 만든 시작/함정/점프/클리어 화면 및 결과
- `artifacts\verification\04-door-entering.png`: 문에 닿은 뒤 자동으로 들어가는 중간 상태
- `artifacts\verification\06-character-poses.png`: 좌우 대기·걷기·점프의 실제 크기와 5배 확대 비교
- `artifacts\stage6-ascent-verification`: 6번 상승 톱날 방의 기믹별 중간 화면과 클리어 검증
- `artifacts\final-two-level-verification`: 2층 보너스 최종 방의 혼합 기믹과 엔딩 검증
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

6번 톱날 방 검증 재실행:

```powershell
$game = Start-Process -FilePath .\bin\Release\LevelDevil.exe `
  -ArgumentList '--verify-stage6', 'artifacts/stage6-ascent-verification' `
  -Wait -PassThru -WindowStyle Hidden
$game.ExitCode
```

보너스 최종 방과 엔딩 검증 재실행:

```powershell
$game = Start-Process -FilePath .\bin\Release\LevelDevil.exe `
  -ArgumentList '--verify-final', 'artifacts/final-two-level-verification' `
  -Wait -PassThru -WindowStyle Hidden
$game.ExitCode
```

소스를 수정한 뒤에는 빌드하고 `Play.cmd`로 실행하세요. `Play.cmd`는 실행 파일이 없을 때만 자동 빌드합니다.

`LevelDevil.exe --verify-motion artifacts/walk-verification/frames`는 실제 입력과 렌더러로 4초 분량의 60fps PNG 시퀀스와 상태 기록을 생성합니다. 영상 인코딩은 검증 자료 제작에만 사용하며 게임 실행에는 필요하지 않습니다.

## 참고

- 사용자 제공 [나무위키 문서](https://namu.wiki/w/Level%20Devil)의 톱날·최종 스테이지 설명과 아래 실제 화면·플레이 영상을 함께 확인했습니다.
- [공식 배포처 Poki — Level Devil](https://poki.com/en/g/level-devil)
- [공식 Steam 페이지의 스크린샷과 예고편 — 캐릭터 외형 참고](https://store.steampowered.com/app/3242750/Level_Devil/)
- [첫 방 1-1 참고 화면](https://zotuf.com/answer/level-devil/1/1-1.jpg)
- [톱날 11-5 참고 화면](https://zotuf.com/answer/level-devil/1/11-5.jpg)
- [톱날 11 스테이지 플레이 영상](https://www.dailymotion.com/video/xa1y00g)
- [최종 16-5 참고 화면](https://zotuf.com/answer/level-devil/1/16-5.jpg)
- [최종 16 스테이지 플레이 영상](https://www.dailymotion.com/video/xa42ac8)
- [방별 플레이 영상](https://zotuf.com/level-devil-walkthrough/)
- [확인한 1-1 영상](https://rutube.ru/play/embed/115f472e01dc626242d86b4bad662b27/?p=K7Zh9dnmy3VDXZXGl0ozuQ)
- [SFML 3.1.0](https://www.sfml-dev.org/download/sfml/3.1.0/)

Level Devil 원작은 Unept의 게임이며, 이 프로젝트는 학습용으로 독립 구현한 팀 프로젝트입니다.
