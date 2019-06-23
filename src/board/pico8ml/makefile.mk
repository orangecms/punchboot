PB_BOARD_NAME = pico8ml
PB_PLAT_NAME  = imx8m
PB_ENTRY      = 0x7E1000

BOARD_C_SRCS += board/pico8ml/board.c
BOARD_C_SRCS += boot/atf_linux.c

KEYS  = ../pki/secp256r1-pub-key.der

KEY_TYPE = EC

