# 스테이지 분담 가이드

## 파일 소유 범위

- 공통 담당자: `Common`, `IStage`, `GameModel`, `RenderCommon`, `StageManager`, `main`, 프로젝트 파일
- 스테이지 담당자: 자기 번호의 `StageX.hpp`, `StageX.cpp`
- 스테이지 담당자는 공통 물리 수치와 다른 조원의 `StageX` 파일을 수정하지 않습니다.
- 6번 담당자의 수정 범위는 `src\Stage6.hpp`, `src\Stage6.cpp` 두 파일입니다. 등록과 엔딩은 공통 담당자가 관리합니다.
- 팀 합의로 추가한 보너스 최종 방은 `src\FinalStage.hpp`, `src\FinalStage.cpp` 두 파일로 분리합니다.
- 효과음은 구현 범위에서 제외합니다. `SFML/Audio.hpp`와 오디오 파일을 추가하지 않습니다.
- 캐릭터 중력 반전, Flappy 조작처럼 캐릭터 자체를 바꾸는 기믹은 사용하지 않고 맵 기믹만 구현합니다.

## 고정 공통 값

`Common.hpp`의 이동 물리는 모든 스테이지에서 그대로 사용합니다. 화면 위쪽 경계만 `ceilingY()`로 방 구조에 맞게 낮출 수 있습니다.

- 내부 화면: 960 x 540
- 플레이어 충돌 상자: 16 x 28
- 좌우 속도: 192
- 중력: 2048
- 점프 속도: 472
- 물리 간격: 1/120초
- 입력: A/D 또는 좌우 방향키, Space/W/위 방향키

## StageX가 구현할 항목

`Stage1.hpp/.cpp`를 구조 예제로 보고 `IStage`의 함수를 구현합니다.

| 함수 | 담당 내용 |
| --- | --- |
| `label()` | 화면에 표시할 스테이지 번호 |
| `spawnPoint()` | 시작 좌표. y는 충돌 상자 왼쪽 위 좌표 |
| `ceilingY()` | 캐릭터가 올라갈 수 있는 화면 위쪽 경계. 구현하지 않으면 공통값 250 사용 |
| `reset()` | 발판, 함정, 문 등 맵 상태 초기화 |
| `update()` | 플레이어 위치에 반응하는 맵 기믹 갱신 |
| `solids()` | 현재 프레임에서 충돌할 벽과 발판 목록 |
| `touchesHazard()` | 가시, 톱날 등 사망 판정 |
| `isAtGoal()` | 출구 자동 진입을 시작할 위치 판정 |
| `goalPoint()` | 문 안으로 들어갈 때 플레이어가 이동할 최종 위치 |
| `draw()` | 배경, 맵, 함정, 문 그리기 |

플레이어 이동, 점프, 중력, 발판 충돌, 낙사, 사망 후 재시작은 `GameModel.cpp`가 처리합니다. 스테이지 파일에서 플레이어 속도나 중력을 다시 계산하지 않습니다.
`isAtGoal()`은 플레이어가 문에 닿았을 때만 `true`를 반환합니다. 이후 문 안으로 걷고 작아지며 사라지는 연출과 실제 클리어 처리는 공통 `GameModel`이 담당합니다.

## 공통 담당자의 연결 작업

스테이지 담당자가 `StageX.hpp/.cpp`를 완성하면 공통 담당자가 다음 세 곳만 갱신합니다.

1. `StageManager.cpp`에서 헤더를 include하고 생성 목록에 `std::make_unique<StageX>()`를 순서대로 추가합니다.
2. `LevelDevil.vcxproj`와 `LevelDevil.vcxproj.filters`에 두 파일을 등록합니다.
3. `tests/GameModelTests.vcxproj`에 `StageX.cpp`를 등록하고 빌드/플레이 경로를 확인합니다.

중간 스테이지를 클리어하면 `StageManager`가 0.65초 뒤 다음 스테이지로 넘깁니다. 마지막 스테이지는 클리어 화면을 유지하고 Enter 또는 버튼으로 전체를 다시 시작합니다.
현재 개발 빌드는 `Stage1 → Stage2 → Stage3 → Stage4 → Stage5 → Stage6 → FinalStage` 순서로 연결되어 있습니다.

## 제출 전 확인

```powershell
.\Build.ps1 -Configuration Debug -Rebuild
.\Build.ps1 -Configuration Release -Rebuild
```

각 스테이지는 최소한 시작 위치, 일반 이동, 기믹 사망, 재시작 후 상태 초기화, 출구 클리어를 직접 플레이해 확인합니다.
