#ifndef COMMON_BOARD_ID_H
#define COMMON_BOARD_ID_H

#include <cfg/compiler.h>

typedef enum CpuId
{
	CI_UNKNOWN = 0,
	CI_STM32F207ZG,
	CI_STM32F207VG,
	CI_STM32F207IG,

	CI_CNT
} CpuId;

#define BOARD_ID_100PIN_CPUMASK 0x8000

typedef enum BoardIds
{
	BOARD_ID_KK348 = 0x0000,
	BOARD_ID_KK353 = BOARD_ID_100PIN_CPUMASK | 0x0000,
	BOARD_ID_KK354 = BOARD_ID_100PIN_CPUMASK | 0x0001,
	BOARD_ID_STM3220G = 0x0007,
} BoardIds;

// Remember to change this when a new board is added
#define BOARD_CNT     4
#define BOARD_DEFAULT (BOARD_CNT - 1) // Use the most recent one added

typedef struct BoardInfo
{
	int id;
	const char name[24];
	CpuId cpu_id;
	const struct AdcPinConf *hw_pin;
} BoardInfo;

int hw_version(void);

int board_id_init(void);
int board_id(void);
const BoardInfo *board_data(int id);
CpuId board_cpu_id(void);
const char *board_cpu(void);
const char *board_name(void);

#define board_findCfg(_id, _cfg)                 \
	({                                           \
		typeof(&((_cfg)[0])) out = NULL;         \
		for (unsigned i = 0; i < BOARD_CNT; i++) \
		{                                        \
			if ((_cfg)[i].id == (_id))           \
			{                                    \
				out = &((_cfg)[i]);              \
				break;                           \
			}                                    \
		}                                        \
		out;                                     \
	})

#endif
