# Project-SSS-Security-Safe-Service
TCPIP Socket 통신 기반 IoT 프로젝트

### 🎯 SSS(Security Safe Service) 프로젝트 목표

RFID 인증과 TCP/IP 소켓 통신을 기반으로 금고 단말을 서버와 연결하여, 인증된 사용자만 안전하게 캐비닛을 사용하고하고 중앙에서 통합 관리할 수 있는 IoT 보안 시스템을 구축하는 것이 목표입니다.
### 🛠 Tech Stack
![C](https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white)
![STM32CubeIDE](https://img.shields.io/badge/STM32CubeIDE-03234B?style=flat&logo=stmicroelectronics&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino%20Uno-00979D?style=flat&logo=arduino&logoColor=white)  
![Firmware](https://img.shields.io/badge/Firmware-0A9396?style=flat)
![TCP/IP Socket](https://img.shields.io/badge/TCP%2FIP%20Socket-005F73?style=flat)
![IoT](https://img.shields.io/badge/IoT-94D2BD?style=flat)
### ✔ 세부 목표
- 가변저항 + 버튼으로 사용자가 비밀번호 입력
- 허가된 사용자만 금고 개폐/사용 가능하게 제어
- TCP/IP 기반 서버로 여러 금고를 통합 관리
- 로그·상태 모니터링 기능을 통한 보안성 강화

### 🎥 시연영상

#### [영상1](https://youtu.be/Lh4SUqPK2ic)
초기 비밀번호 0000  
사용자가 0000 입력 --> 로그인 성공  
관리자가 비밀번호 1111로 변경  
사용자가 0000 입력 --> 로그인 실패  
사용자가 1111 입력 --> 로그인 성공  

#### [영상2](https://youtu.be/F2MfLSNTfTs)
비밀번호 5번 연속 인증 실패 시 **lock**  
도난 감지(볼케이스)시 **lock**

---

## 🏗️ 시스템 구성 

<img width="853" height="454" alt="image" src="https://github.com/user-attachments/assets/5c6b7f82-ee05-4ccc-a111-b4134b04a2b3" />  

<img width="911" height="497" alt="image" src="https://github.com/user-attachments/assets/2531f0c8-6278-4094-b760-906ba561241e" />

### 📡 여러 캐비닛이 서버로 비밀번호를 보내고, 서버가 DB와 관리자 시스템을 통해 개방 여부를 관리하는 구조
| 구성 요소 | 역할 | 연결 방식 |
|----------|------|-----------|
| **금고** | 인증 요청/입력 장치 | Wi-Fi → 서버 |
| **서버 (라즈베리파이)** | 금고 관리, BLE 송신, DB 연동 | Wi-Fi ↔ 금고<br>BLE ↔ 관리자 알림 시스템<br>SQL ↔ MariaDB |
| **MariaDB (DB 클라이언트)** | 금고 데이터베이스 저장/조회 | 서버 ↔ DB |
| **관리자 Application** | 금고 상태 조회, 알림 확인 | 서버 ↔ Application |
| **관리자 알림 시스템 (Arduino)** | 금고 알림 수신 (BlueTooth) | 서버 → BlueTooth 장치 |

---

## 🔐 비밀번호 기반 인증 시스템

다이얼 금고에서 입력된 비밀번호를 서버로 전달해
DB에 저장된 비밀번호와 비교 후 **정상 인증 / 실패** 여부를 반환하는 구조입니다.

### 🔎 인증 절차
1. 사용자가 다이얼 금고에 비밀번호 입력  
2. 금고 → 서버로 인증 요청 전송  
3. 서버는 DB Client를 통해 저장된 비밀번호 조회  
4. **일치 시 성공**, 불일치 시 실패로 응답  
5. 실패가 **5회 누적되면 자동 System Lock 발생**

---

## ⚠️ 보안 기능 (Security Features)

### 🔐 1) System Lock (5회 실패 시 자동 잠금)
- 동일 ID가 비밀번호 입력에 5회 실패하면 계정이 잠금 상태가 됨  
- 관리자가 별도 명령을 통해 잠금 해제 가능  
- 잠금 시도는 모두 로그로 기록됨

### 📝 2) 로그인 이벤트 로그 기록
모든 인증 시도는 DB의 Safe_Log 테이블에 저장됩니다.
- 성공 / 실패 / 잠금 발생 등 보안 이벤트 추적 가능
- 불법 접근 감지에 활용

---

## 🚨 보안 위험 이벤트 알림

금고에서 **비밀번호 연속 실패(5회)** 또는 **의심 이벤트** 발생 시:
- 금고 → 서버로 위험 이벤트 보고  
- 서버는 이벤트를 DB에 기록  
- 부저(Arduino)로 경고음 출력, LCD에 경고 메시지 표시  

---

## 🛠 관리자 기능

### 🗝️ 비밀번호 변경
관리자 앱에서 비밀번호를 변경하면 DB의 Safe_Info 테이블이 즉시 갱신됩니다.

### 🔓 시스템 잠금 해제
비밀번호 5회 오류로 잠긴 계정을 관리자 명령을 통해 해제할 수 있습니다.

---

## 📦 주요 DB 테이블 요약

### 🔐 Safe_Info  
| ID | PASSWORD | SYSTEM_LOCK | FAIL_COUNT |
|----|----------|-------------|------------|
| 사용자의 비밀번호 및 상태 정보 저장 |

### 📜 Safe_Log  
| ID | LOG | DATE | TIME |
|----|-----|------|------|
| 로그인 성공/실패, 잠금 발생 등 보안 로그 저장 |

---

## 📘 요약

- 비밀번호 입력 → 서버 인증 → 결과 반환  
- 비밀번호 5회 실패 시 **자동 System Lock**  
- 모든 인증 이벤트는 **로그로 저장**  
- 보안 위협 발생 시 **경고음·LCD 출력 + 서버 저장**  
- 관리자에게 **비밀번호 변경·잠금 해제 기능 제공**






## 🧯 Troubleshooting – Wi-Fi 인터럽트로 인한 시스템 정지
각 캐비닛을 담당하는 STM32에 Wi-Fi 모듈을 달고, 수신 신호를 외부 인터럽트로 처리했는데,
인터럽트 핸들러 안에서 패킷 처리까지 모두 수행하면서 CPU를 너무 오래 점유해 버림.
그 결과, 다른 캐비닛 제어 루틴이 거의 돌지 않는 문제가 발생했다.

구조를 바꿔서, 인터럽트에서는 플래그만 세팅하고 바로 리턴하도록 수정하고,
메인 루프에서 플래그를 확인해 실제 처리를 수행하는 방식으로 변경했다.

이 경험을 통해, MCU에서는 인터럽트 핸들러를 최대한 짧게 유지하고
실제 작업은 메인 루프나 별도 태스크로 넘겨야 한다는 점을 몸으로 배웠다.
