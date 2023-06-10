# 개발 환경 설정
- linux-5.15.114 디렉토리가 준비된 상태에서 진행
- linux-5.15.114 디렉토리로 이동
- config 구성
    ```shell
    make CC=clang defconfig
    ````
- kernel/Makefile의 obj-y에 mychat.o 추가
- kernel 아래에 빈 mychat.c 파일 추가
- make 실행