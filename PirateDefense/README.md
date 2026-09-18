# Pirate Defense

C++ / SFML / TCP/IP 기반의 **최대 5인 협동 디펜스 게임**입니다.  
플레이어들은 해적선을 방어하면서 적과 보스를 상대하고, 낚시·아이템·상점·업그레이드 시스템을 활용해 스테이지를 진행합니다.

## 주요 기술

- **Language**: C++
- **Graphics / UI**: SFML 3.1.0
- **Network**: TCP/IP (`sf::TcpListener`, `sf::TcpSocket`)
- **Platform**: Windows / Visual Studio 2022
- **Architecture**: Host-authoritative multiplayer synchronization

## 주요 기능

- 닉네임 설정 및 메인 메뉴
- 방 생성 / 6자리 방 코드 참가
- 최대 5인 TCP/IP 멀티플레이
- 플레이어 입력 및 게임 상태 동기화
- 실시간 한글/영문 채팅
- 낮 → 저녁 → 밤 환경 변화
- 밤 시간 플레이어 중심 시야 제한
- 다단계 스테이지 / 적 / 보스 전투
- 낚시 및 회복 시스템
- 스테이지 보상, 상점, 탄종 및 업그레이드
- 게임 UI/HUD 및 엔딩 연출

## 김초아 — 담당 역할

프로젝트에서 아래 부분을 담당했습니다. 관련 핵심 코드에는 `[김초아 담당]` 주석을 추가해 구현 위치를 바로 확인할 수 있도록 정리했습니다.

| 담당 영역 | 구현 내용 | 주요 파일 |
|---|---|---|
| TCP/IP 네트워크 연동 | 방 생성/검색/접속, 로비 동기화, 플레이어 입력 및 게임 상태 송수신 | `NetworkManager.h`, `NetworkManager.cpp`, `MultiGame.cpp` |
| 실시간 채팅 | TCP/IP 채팅 송수신, 닉네임 표시, Windows IME 기반 한글 입력, 채팅 UI 연결 | `NetworkManager.*`, `main.cpp`, `Render.hpp` |
| 메인 SFML 시스템 | 화면 상태 관리, 닉네임/메뉴/방 생성·참가/로비/게임 화면 전환 | `main.cpp` |
| 코드 통합 | 네트워크와 게임 루프, 입력, Renderer 및 팀원 구현 기능 통합 | `main.cpp`, `MultiGame.cpp` |
| 야간 시야 제한 | 플레이어 중심 가시 반경과 거리 기반 암부 페이드 구현 | `Render.hpp` |
| UI / 전체 디자인 | 메인 화면, 로비, HUD, 채팅 및 게임 전반의 화면 구성과 디자인 통합 | `main.cpp`, `Render.hpp` |

## 담당 코드 찾는 방법

소스에서 아래 키워드를 검색하면 개인 담당 부분을 빠르게 확인할 수 있습니다.

```text
[김초아 담당]
```

대표적인 확인 위치:

- `NetworkManager.cpp` — TCP/IP 연결, 패킷 처리, 채팅, 게임 동기화
- `main.cpp` — SFML 메인 시스템, 화면 전환, 채팅 입력 및 UI 연동
- `MultiGame.cpp` — 네트워크와 실제 게임 업데이트 루프 통합
- `Render.hpp::drawNightVision()` — 야간 시야 제한 효과

## 프로젝트 구조

```text
PirateDefense/
├─ main.cpp                 # 메인 SFML 시스템 / 화면 및 UI 통합
├─ NetworkManager.*         # TCP/IP 네트워크 / 채팅 / 동기화
├─ MultiGame.*              # 멀티플레이 게임 루프 연결
├─ SingleGame.*             # 싱글플레이 실행 모드
├─ Game.hpp                 # 핵심 게임 로직 및 상태
├─ Render.hpp               # 게임 렌더링 / HUD / 야간 시야
├─ SpriteAnimation.hpp      # 스프라이트 애니메이션
├─ EndingCutscene.hpp       # 엔딩 연출
├─ Items/                   # 상점 / 골드 / 탄종 / 업그레이드
├─ assets/                  # 이미지 및 게임 리소스
└─ Runtime/                 # 실행 파일 및 필요한 DLL
```

## 빌드 환경

- Visual Studio 2022
- x64
- SFML 3.1.0
- 기본 SFML 경로: `C:\SFML\SFML-3.1.0`

Visual Studio에서 `PirateDefense.sln`을 열어 빌드할 수 있습니다. 다른 위치에 SFML을 설치한 경우 프로젝트의 Include/Library 경로를 변경해야 합니다.

## 실행

빌드 없이 포함된 실행 파일을 사용할 경우 프로젝트 루트의 `run.cmd`를 실행합니다. `Runtime` 폴더와 `assets` 폴더의 상대 경로를 유지해야 합니다.

## Repository 정리

GitHub에는 Visual Studio의 `.vs`, `x64`, 중간 오브젝트/PDB 등 로컬 빌드 산출물을 제외하도록 `.gitignore`를 포함했습니다.
