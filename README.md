# Project-SSS-Security-Safe-Service
TCPIP Socket 통신 기반 IoT 프로젝트

## 🎯 SSS(Security Safe Service) 프로젝트 목표

RFID 인증과 TCP/IP 소켓 통신을 기반으로 금고 단말을 서버와 연결하여, 인증된 사용자만 안전하게 캐비닛을 사용하고하고 중앙에서 통합 관리할 수 있는 IoT 보안 시스템을 구축하는 것이 목표입니다.

### ✔ 세부 목표
- 가변저항 + 버튼으로 사용자가 비밀번호 입력
- 허가된 사용자만 금고 개폐/사용 가능하게 제어
- TCP/IP 기반 서버로 여러 금고를 통합 관리
- 로그·상태 모니터링 기능을 통한 보안성 강화

### 🎯 시연영상

#### [영상1](https://youtu.be/Lh4SUqPK2ic)
초기 비밀번호 0000  
사용자가 0000 입력 --> 로그인 성공  
관리자가 비밀번호 1111로 변경  
사용자가 0000 입력 --> 로그인 실패  
사용자가 1111 입력 --> 로그인 성공  

#### [영상2](https://youtu.be/F2MfLSNTfTs)
비밀번호 5번 연속 인증 실패 시 **lock**  
도난 감지(볼케이스)시 **lock**


## 시스템 구성 


<img width="853" height="454" alt="image" src="https://github.com/user-attachments/assets/5c6b7f82-ee05-4ccc-a111-b4134b04a2b3" />  


<img width="911" height="497" alt="image" src="https://github.com/user-attachments/assets/2531f0c8-6278-4094-b760-906ba561241e" />


### 여러 캐비닛이 서버로 비밀번호를 보내고, 서버가 DB와 관리자 시스템을 통해 개방 여부를 관리하는 구조
| 구성 요소 | 역할 | 연결 방식 |
|----------|------|-----------|
| **금고** | 인증 요청/입력 장치 | Wi-Fi → 서버 |
| **서버 (라즈베리파이)** | 금고 관리, BLE 송신, DB 연동 | Wi-Fi ↔ 금고<br>BLE ↔ 관리자 알림 시스템<br>SQL ↔ MariaDB |
| **MariaDB (DB 클라이언트)** | 금고 데이터베이스 저장/조회 | 서버 ↔ DB |
| **관리자 Application** | 금고 상태 조회, 알림 확인 | 서버 ↔ Application |
| **관리자 알림 시스템 (Arduino)** | 금고 알림 수신 (BlueTooth) | 서버 → BlueTooth 장치 |


### 주요 개발 내용 및 기술
- TCPIP서버, 클라이언트 개발
- DB설계 및 관리 소스코드 개발
- 캐비닛 wi-fi 통신 구현
- 인증 시스템 개발
