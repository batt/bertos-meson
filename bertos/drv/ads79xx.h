#ifndef ADS79XX_H
#define ADS79XX_H

#include "ads79xx_macros.h"
#include "cfg/cfg_ads79xx.h"

#include <cfg/compiler.h>
#include <drv/adc.h>
#include <net/tcp_socket.h>

#include <mware/event.h>

#define ADC_TCP_SAMPLE_COUNT 192
#define SAMPLE_RATE_RATIO    (CONFIG_ADS79XX_SAMPLE_RATE / CONFIG_ADCTCP_SAMPLE_RATE)
STATIC_ASSERT(CONFIG_ADS79XX_SAMPLE_RATE % CONFIG_ADCTCP_SAMPLE_RATE == 0);

// TODO: AUTO2 not handled
typedef enum
{
	ADS_MANUAL,
	ADS_AUTO1,
} Ads79xxMode;

typedef enum
{
	ADS_RANGE25 = 0, /// Selects +2.5V i/p range
	ADS_RANGE50 = 1, /// Selects +5V i/p range
} Ads79xxRange;

typedef enum
{
	ADC_STREAMING,
	ADC_FILL_BUF,
} AdcMode;

typedef struct Ads79xx
{
	AdcContext adc;
	uint32_t mask;
	AdcMode mode;
	Event dma_done;
	Ads79xxRange range;
} Ads79xx;

#define ADC_ADS79XX MAKE_ID('A', 'D', '7', '9')

INLINE Ads79xx *ADS79XX_CAST(AdcContext *ctx)
{
	ASSERT(ctx->_type == ADC_ADS79XX);
	return (Ads79xx *)ctx;
}

void ads79xx_init(Ads79xx *ctx, Ads79xxRange range);
void adc_reset(void);

typedef struct Ads79xxTcp
{
	Ads79xx ads;
	TcpSocket insock;
	uint8_t *volatile tail_tcp;
	uint8_t *volatile head_tcp;
	uint8_t *tcp_buf;
	size_t tcpbuf_len;
	Event buffer_ready;
	int server_sock, in_sock;
} Ads79xxTcp;

#define ADC_ADS79XXTCP MAKE_ID('A', 'T', 'C', 'P')

INLINE Ads79xxTcp *ADS79XXTCP_CAST(AdcContext *ctx)
{
	return (Ads79xxTcp *)ctx;
}

void ads79xxtcp_init(Ads79xxTcp *ctx, Ads79xxRange range, uint8_t *tcp_buf, size_t tcpbuf_len);

#endif //ADS79XX_H
