#ifndef SEQ_H
#define SEQ_H

#include <cfg/compiler.h>

#define SEQ(x)  PP_CAT(_SEQ_, x)
#define _SEQ_1  0
#define _SEQ_2  _SEQ_1, 1
#define _SEQ_3  _SEQ_2, 2
#define _SEQ_4  _SEQ_3, 3
#define _SEQ_5  _SEQ_4, 4
#define _SEQ_6  _SEQ_5, 5
#define _SEQ_7  _SEQ_6, 6
#define _SEQ_8  _SEQ_7, 7
#define _SEQ_9  _SEQ_8, 8
#define _SEQ_10 _SEQ_9, 9
#define _SEQ_11 _SEQ_10, 10
#define _SEQ_12 _SEQ_11, 11
#define _SEQ_13 _SEQ_12, 12
#define _SEQ_14 _SEQ_13, 13
#define _SEQ_15 _SEQ_14, 14
#define _SEQ_16 _SEQ_15, 15
#define _SEQ_17 _SEQ_16, 16
#define _SEQ_18 _SEQ_17, 17
#define _SEQ_19 _SEQ_18, 18
#define _SEQ_20 _SEQ_19, 19
#define _SEQ_21 _SEQ_20, 20
#define _SEQ_22 _SEQ_21, 21
#define _SEQ_23 _SEQ_22, 22
#define _SEQ_24 _SEQ_23, 23
#define _SEQ_25 _SEQ_24, 24
#define _SEQ_26 _SEQ_25, 25
#define _SEQ_27 _SEQ_26, 26
#define _SEQ_28 _SEQ_27, 27
#define _SEQ_29 _SEQ_28, 28
#define _SEQ_30 _SEQ_29, 29
#define _SEQ_31 _SEQ_30, 30
#define _SEQ_32 _SEQ_31, 31

#define CALL(b, x) b(x)

#define _FOR_N(body, n, ...) \
	PP_CAT(FOR_N_, n)        \
	(body, __VA_ARGS__)

#define FOR_N(body, n) \
	_FOR_N(body, n, SEQ(n))

#define FOR_N_1(body, x) \
	CALL(body, x)

#define FOR_N_2(body, x, ...) \
	CALL(body, x)             \
	FOR_N_1(body, __VA_ARGS__)

#define FOR_N_3(body, x, ...) \
	CALL(body, x)             \
	FOR_N_2(body, __VA_ARGS__)

#define FOR_N_4(body, x, ...) \
	CALL(body, x)             \
	FOR_N_3(body, __VA_ARGS__)

#define FOR_N_5(body, x, ...) \
	CALL(body, x)             \
	FOR_N_4(body, __VA_ARGS__)

#define FOR_N_6(body, x, ...) \
	CALL(body, x)             \
	FOR_N_5(body, __VA_ARGS__)

#define FOR_N_7(body, x, ...) \
	CALL(body, x)             \
	FOR_N_6(body, __VA_ARGS__)

#define FOR_N_8(body, x, ...) \
	CALL(body, x)             \
	FOR_N_7(body, __VA_ARGS__)

#define FOR_N_9(body, x, ...) \
	CALL(body, x)             \
	FOR_N_8(body, __VA_ARGS__)

#define FOR_N_10(body, x, ...) \
	CALL(body, x)              \
	FOR_N_9(body, __VA_ARGS__)

#define FOR_N_11(body, x, ...) \
	CALL(body, x)              \
	FOR_N_10(body, __VA_ARGS__)

#define FOR_N_12(body, x, ...) \
	CALL(body, x)              \
	FOR_N_11(body, __VA_ARGS__)

#define FOR_N_13(body, x, ...) \
	CALL(body, x)              \
	FOR_N_12(body, __VA_ARGS__)

#define FOR_N_14(body, x, ...) \
	CALL(body, x)              \
	FOR_N_13(body, __VA_ARGS__)

#define FOR_N_15(body, x, ...) \
	CALL(body, x)              \
	FOR_N_14(body, __VA_ARGS__)

#define FOR_N_16(body, x, ...) \
	CALL(body, x)              \
	FOR_N_15(body, __VA_ARGS__)

#define FOR_N_17(body, x, ...) \
	CALL(body, x)              \
	FOR_N_16(body, __VA_ARGS__)

#define FOR_N_18(body, x, ...) \
	CALL(body, x)              \
	FOR_N_17(body, __VA_ARGS__)

#define FOR_N_19(body, x, ...) \
	CALL(body, x)              \
	FOR_N_18(body, __VA_ARGS__)

#define FOR_N_20(body, x, ...) \
	CALL(body, x)              \
	FOR_N_19(body, __VA_ARGS__)

#define FOR_N_21(body, x, ...) \
	CALL(body, x)              \
	FOR_N_20(body, __VA_ARGS__)

#define FOR_N_22(body, x, ...) \
	CALL(body, x)              \
	FOR_N_21(body, __VA_ARGS__)

#define FOR_N_23(body, x, ...) \
	CALL(body, x)              \
	FOR_N_22(body, __VA_ARGS__)

#define FOR_N_24(body, x, ...) \
	CALL(body, x)              \
	FOR_N_23(body, __VA_ARGS__)

#define FOR_N_25(body, x, ...) \
	CALL(body, x)              \
	FOR_N_24(body, __VA_ARGS__)

#define FOR_N_26(body, x, ...) \
	CALL(body, x)              \
	FOR_N_25(body, __VA_ARGS__)

#define FOR_N_27(body, x, ...) \
	CALL(body, x)              \
	FOR_N_26(body, __VA_ARGS__)

#define FOR_N_28(body, x, ...) \
	CALL(body, x)              \
	FOR_N_27(body, __VA_ARGS__)

#define FOR_N_29(body, x, ...) \
	CALL(body, x)              \
	FOR_N_28(body, __VA_ARGS__)

#define FOR_N_30(body, x, ...) \
	CALL(body, x)              \
	FOR_N_29(body, __VA_ARGS__)

#define FOR_N_31(body, x, ...) \
	CALL(body, x)              \
	FOR_N_30(body, __VA_ARGS__)

#define FOR_N_32(body, x, ...) \
	CALL(body, x)              \
	FOR_N_31(body, __VA_ARGS__)

#endif
