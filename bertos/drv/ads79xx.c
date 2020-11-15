#include "ads79xx.h"

#if CPU_ARM
	#include <io/arm.h>
#else
	#include <io/cm3.h>
	#include <drv/gpio_stm32.h>
	#include <drv/irq_cm3.h>
#endif
#include <cfg/macros.h> // BV
#include <cfg/debug.h>  // ASSERT
#include <cpu/irq.h>    // DECLARE_ISR
#include <mware/event.h>
#include <kern/proc.h>
#include <net/tcp_socket.h>
#include <kern/msg.h>
#include <drv/timer.h> // timer_delay

#include <string.h> // memset

#define LOG_LEVEL  LOG_LVL_INFO
#define LOG_FORMAT LOG_FMT_TERSE
#include <cfg/log.h>

#define ADS_MANUAL_MODE(ch, range) (0x1000 | (ch & 0xF) << 7 | range << 6)
#define ADS_PROGRAM_AUTO1          (0x8000)
#define ADS_AUTO1_RESET_SEQ(range) (0x2C00 | range << 6)
#define ADS_AUTO1(range)           (0x2800 | range << 6)

#if CPU_ARM_AT91

    // Set ADC_TCP_STROBE to 1 in order to enable debugging of adc isr duration.
	#define ADC_TCP_STROBE 1
	#if ADC_TCP_STROBE
		#define STROBE_PIN            BV(23)
		#define ADC_TCP_STROBE_LOW()  (PIOB_CODR = STROBE_PIN)
		#define ADC_TCP_STROBE_HIGH() (PIOB_SODR = STROBE_PIN)
		#define ADC_TCP_STROBE_INIT()   \
			do                          \
			{                           \
				PIOB_OER = STROBE_PIN;  \
				PIOB_SODR = STROBE_PIN; \
				PIOB_PER = STROBE_PIN;  \
			} while (0)
	#else
		#define ADC_TCP_STROBE_LOW()
		#define ADC_TCP_STROBE_HIGH()
		#define ADC_TCP_STROBE_INIT()
	#endif

	#define WAIT_READY()                      \
		do                                    \
		{                                     \
			while (!(SSC_SR & BV(SSC_TXRDY))) \
				;                             \
		} while (0)

	#define ADC_WRITE(value)        \
		SSC_THR = (int16_t)(value); \
		WAIT_READY()

    /*
 * Transfer 2 empty frames to synchronize with hardware.
 * It takes a while before the ADC starts emitting
 * converted frames with the new mode
 */
	#define HW_SYNCH()                          \
		do                                      \
		{                                       \
			WAIT_READY();                       \
			SSC_THR = 0;                        \
			WAIT_READY();                       \
			SSC_THR = 0;                        \
			while (!(SSC_SR & BV(SSC_TXEMPTY))) \
				;                               \
		} while (0)

#else

	#define DMA_STREAM  3
	#define DMA_CHANNEL 0
	#define DMA_IRQ     DMA1STREAM3_IRQHANDLER

	#define WAIT_HALF_CLOCK() \
		timer_busyWait(us_to_hptime(1))

	#define CS_UP() \
		stm32_gpioPinWrite(GPIOB, BV(12), true);

	#define CS_DOWN() \
		stm32_gpioPinWrite(GPIOB, BV(12), false);

    /* since there is a NOT port on the clock pin, definitions are inverted */
	#define CLOCK_UP() \
		stm32_gpioPinWrite(GPIOB, BV(13), false)

	#define CLOCK_DOWN() \
		stm32_gpioPinWrite(GPIOB, BV(13), true)

	#define WRITE_DATA(d) \
		stm32_gpioPinWrite(GPIOA, BV(9), (d));

static void ADC_WRITE(uint16_t value)
{
	CLOCK_UP();
	WAIT_HALF_CLOCK();

	CS_DOWN();

	for (int i = 0; i < 16; ++i)
	{
		WRITE_DATA(value >> 15);
		value <<= 1;

		CLOCK_DOWN();
		WAIT_HALF_CLOCK();

		CLOCK_UP();
		WAIT_HALF_CLOCK();
	}

	CS_UP();

	CLOCK_DOWN();
	WAIT_HALF_CLOCK();
}

	#define ADC_READ()                        \
		({                                    \
			while (!(SPI2->SR & SPI_SR_RXNE)) \
				;                             \
			SPI2->DR;                         \
		})

#endif

/* Divide slicelen by sample size to obtain sample count (samples are 16-bit) */
#define SLICE_TO_SAMPLES(slicelen) (slicelen >> 1)

/* len must be an exact divisor of buf_len */
#define MOVE_AND_WRAP(ptr, len, base_buf, buf_len) \
	do                                             \
	{                                              \
		(ptr) += (len);                            \
		if ((ptr) == (base_buf) + (buf_len))       \
			(ptr) = (base_buf);                    \
	} while (0)

/* TODO: more events are needed when more Ads79xx contexts can be created */
static void *handle;

static volatile int cnt = 0;
static DECLARE_ISR(ads_dmaIrqHandler)
{
	AdcContext *ctx = &((Ads79xx *)handle)->adc;
	uint8_t *tail = ctx->tail;
	uint8_t *sliceend = ctx->sliceend;

	size_t len;
	if (ctx->streaming_hook)
	{
		len = ctx->hook_slicelen;
		ctx->streaming_hook(ctx, tail, len);
	}
	else
		len = ctx->slicelen;

	MOVE_AND_WRAP(tail, len, ctx->base_buf, ctx->buf_len);

	uint8_t *mem = tail;
	MOVE_AND_WRAP(mem, len, ctx->base_buf, ctx->buf_len);
	/* set next dma transfer */
#if CPU_ARM_AT91
	SSC_RNPR = (reg32_t)mem;
	SSC_RNCR = SLICE_TO_SAMPLES(len);
#else
	if (DMA1->STR[DMA_STREAM].CR & DMA_CR_TARGET_1)
		DMA1->STR[DMA_STREAM].M0AR = (reg32_t)mem;
	else
		DMA1->STR[DMA_STREAM].M1AR = (reg32_t)mem;
#endif

	/* Signal only when the slicelen is ready */
	if (tail == sliceend)
	{
		MOVE_AND_WRAP(sliceend, ctx->slicelen, ctx->base_buf, ctx->buf_len);
		/* Update tail valid samples */
		cnt++;
		event_do(&((Ads79xx *)handle)->dma_done);
	}

	ctx->tail = tail;
	ctx->sliceend = sliceend;
#if CPU_ARM_AT91
	AIC_EOICR = 0;
#else
	DMA_CLEAR_TCI(DMA1, DMA_STREAM);
#endif
}

void adc_reset(void)
{
	cnt = 0;
}

static void *ads79xx_wait(AdcContext *ctx)
{
	Ads79xx *_ctx = ADS79XX_CAST(ctx);
	uint8_t *ret_buf;
	DB(static ticks_t start = 0);

	switch (_ctx->mode)
	{
	case ADC_STREAMING:
#if _DEBUG
		if (cnt != 0)
		{
			LOG_ERR("Assertion failed: ADC overrun, cnt %d, time %dms\n", cnt, ticks_to_ms(timer_clock() - start));
			ASSERT(cnt == 0);
		}
		start = timer_clock();
#endif
		event_wait(&_ctx->dma_done);
		// TODO: if this assert fails, the condition has to be handled!
		// (time stamp in reduction should be adjusted)
		ASSERT(cnt == 1);
		cnt = 0;

		ret_buf = ctx->head;
		/* move head forward */
		MOVE_AND_WRAP(ctx->head, ctx->slicelen, ctx->base_buf, ctx->buf_len);
		break;
#if 0
	case ADC_FILL_BUF:
		event_wait(&e);
		ret_buf = _ctx->buf;
		break;
#endif
	default:
		ASSERT(0);
		ret_buf = NULL;
		break;
	}
	return ret_buf;
}

static void ads79xx_dmaStop(UNUSED_ARG(AdcContext *, ctx))
{
#if CPU_ARM_AT91
	/* Disable DMA IRQ */
	SSC_IDR = BV(SSC_ENDRX);
	SSC_PTCR = BV(PDC_RXTDIS);
#else
	/* disable RX DMA */
	SPI2->CR2 &= ~SPI_CR2_RXDMAEN;
	DMA1->STR[DMA_STREAM].CR &= ~DMA_CR_EN;
	/* disable I2S periperal */
	SPI2->I2SCFGR &= ~SPI_I2SCFGR_I2SE;
	/* deconfigure GPIOs */
	stm32_gpioPinConfig(GPIOB, BV(12) | BV(13) | BV(15),
	                    GPIO_MODE_IN_FLOATING, GPIO_SPEED_50MHZ);
	stm32_gpioPinConfig(GPIOA, BV(9),
	                    GPIO_MODE_IN_FLOATING, GPIO_SPEED_50MHZ);
#endif
}

static void ads79xx_dmaStart(AdcContext *ctx)
{
	Ads79xx *_ctx = ADS79XX_CAST(ctx);

	/* must be called only with DMA transfers disabled */
#if CPU_ARM_AT91
	ASSERT(!(SSC_PTSR & BV(PDC_RXTEN)));
#else
	ASSERT(!(SPI2->I2SCFGR & SPI_I2SCFGR_I2SE));
#endif
	cnt = 0;
	event_initGeneric(&_ctx->dma_done);

	ctx->head = ctx->tail = ctx->base_buf;
	ctx->sliceend = ctx->tail + ctx->slicelen;
	_ctx->mode = ADC_STREAMING;
	handle = _ctx;

	/* compute start address and byte count */
#if CPU_ARM_AT91
	SSC_RPR = (reg32_t)ctx->tail;
#else
	DMA1->STR[DMA_STREAM].M0AR = (reg32_t)ctx->tail;
#endif
	size_t len;
	if (ctx->streaming_hook)
		len = ctx->hook_slicelen;
	else
		len = ctx->slicelen;
	/* len is in bytes but samples are 16 bits, so it must be a integer multiple of 2 bytes */
	ASSERT(len % 2 == 0);

#if CPU_ARM_AT91
	SSC_RCR = SLICE_TO_SAMPLES(len);
	SSC_RNPR = (reg32_t)(ctx->tail + len);
	SSC_RNCR = SLICE_TO_SAMPLES(len);

	/* Enable DMA IRQ */
	SSC_IER = BV(SSC_ENDRX);
#else
	DMA1->STR[DMA_STREAM].NDTR = SLICE_TO_SAMPLES(len);
	DMA1->STR[DMA_STREAM].M1AR = (reg32_t)(ctx->tail + len);

	/* enable GPIO B, A */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOAEN;

	stm32_gpioPinConfig(GPIOB, BV(12) | BV(13) | BV(14),
	                    GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);
	stm32_gpioPinConfig(GPIOA, BV(9),
	                    GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);

	CS_UP();
	CLOCK_DOWN();
	WRITE_DATA(0);
#endif

	/* If the user is calling setChannel, she wants to use the streaming API */
	ATOMIC(
	    ADC_WRITE(ADS_PROGRAM_AUTO1);
	    ADC_WRITE(_ctx->mask & 0x0000FFFF);
	    ADC_WRITE(ADS_AUTO1_RESET_SEQ(_ctx->range));
	    ADC_WRITE(ADS_AUTO1(_ctx->range));
	    ADC_WRITE(ADS_AUTO1(_ctx->range));
#if CPU_ARM_AT91
	    /*
		  * RX and TX sections of the SSC are NOT synchronized, so even if we
		  * have just wait TXRDY, this does not guarantee RXRDY will be set.
		  *
		  * In order to be sure the receiver is in a steady state, well away from
		  * the end of a reception, we add a small delay.
		  * After this, we can for sure assume no reception is ongoing and
		  * we can clear the RXRDY flag by reading SSC_RHR.
		  */
	    for (int i = 0; i < 10; ++i)
	        NOP;

	    (void)SSC_RHR;

	    SSC_PTCR = BV(PDC_RXTEN););
#else
	);
	/* last channel in sequence */
	int last_ch;
	for (last_ch = 15; last_ch >= 0; --last_ch)
		if (_ctx->mask & BV(last_ch))
			break;

	/* enable GPIO B, A */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOAEN;

	/* PB14 is used by SPI but not by I2S */
	stm32_gpioPinConfig(GPIOB, BV(14),
	                    GPIO_MODE_IN_FLOATING, GPIO_SPEED_50MHZ);

	/* divide I2SCLOCK by 5: */
	SPI2->I2SPR &= ~SPI_I2S_CLOCKMASK;
	SPI2->I2SPR |= SPI_I2S_CLOCKDIV(5);

	/* enable I2S mode, master receive, short frame, PCM, CKPOL low, 16 bit data, 32 bit frame */
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SMODE | SPI_I2SCFGR_I2SCFG_RXMASTER | SPI_I2SCFGR_CKPOL |
	                 SPI_I2SCFGR_I2SSTD_PCM | SPI_I2SCFGR_DATLEN_16BIT | SPI_I2SCFGR_CHLEN_32BIT;

	/* enable peripheral as last step, but before configuring GPIO alternate function */
	SPI2->I2SCFGR |= SPI_I2SCFGR_I2SE;

	stm32_gpioPinConfig(GPIOB, BV(12) | BV(13) | BV(15),
	                    GPIO_MODE_AF_PP | GPIO_AF_I2S2, GPIO_SPEED_50MHZ);

	/* the first 1-2 frames we receive are garbage */
	for (int i = 0; i < 4; ++i)
		ADC_READ();

	/* wait for the last channel in the sequence */
	for (int i = 0; i < 16; ++i)
	{
		uint16_t val = ADC_READ();

		if (ADS79xx_GET_CHAN(val) == last_ch)
			break;
	}

	/* enable RX DMA */
	SPI2->CR2 |= SPI_CR2_RXDMAEN;
	DMA1->STR[DMA_STREAM].CR |= DMA_CR_EN;
#endif
}

/*
 * You must call this function when dma transfers are disabled.
 */
static void ads79xx_setChannel(AdcContext *ctx, uint32_t mask)
{
	Ads79xx *_ctx = ADS79XX_CAST(ctx);
	_ctx->mask = mask;
}

#if CPU_ARM_AT91

/*
 * Read a value from the ADC.
 */
static adcread_t ads79xx_read(AdcContext *ctx, adc_ch_t channel)
{
	Ads79xx *_ctx = ADS79XX_CAST(ctx);

	/* set manual mode on channel ch */
	SSC_THR = ADS_MANUAL_MODE(channel, _ctx->range);
	HW_SYNCH();

	while (!(SSC_SR & BV(SSC_RXRDY)))
		;
	adcread_t val = SSC_RHR;

	/* set again the previous mask */
	ads79xx_setChannel(ctx, _ctx->mask);

	return val;
}

#endif

#define DATALEN (15 & SSC_DATLEN_MASK)
#define DELAY   ((0 << SSC_STTDLY_SHIFT) & SSC_STTDLY_MASK)
#define PERIOD  ((((16 + CONFIG_ADS79XX_PAD) / 2 - 1) << (SSC_PERIOD_SHIFT)) & SSC_PERIOD_MASK)
#define DATNB   ((0 << SSC_DATNB_SHIFT) & SSC_DATNB_MASK)
#define FSLEN   (((CONFIG_ADS79XX_PAD - 1) << SSC_FSLEN_SHIFT) & SSC_FSLEN_MASK)

#define MCK_DIV(sample_freq, channels) (CPU_FREQ / ((sample_freq) * (channels) * (16 + CONFIG_ADS79XX_PAD) * 2))

#define ADS_DMA_IRQ_PRIORITY 4
void ads79xx_init(Ads79xx *ctx, Ads79xxRange range)
{
	memset(ctx, 0, sizeof(*ctx));
	event_initGeneric(&ctx->dma_done);
	ctx->range = range;

#if CPU_ARM_AT91
	// set up SSC
	PIOA_PDR = BV(SSC_TK) | BV(SSC_TF) | BV(SSC_TD);
	PIOA_PDR = BV(SSC_RD);
	SSC_CR = BV(SSC_SWRST);

	STATIC_ASSERT((CPU_FREQ / MCK_DIV(CONFIG_ADS79XX_SAMPLE_RATE, CONFIG_ADS79XX_CHANNELS)) == (2 * CONFIG_ADS79XX_SAMPLE_RATE * (16 + CONFIG_ADS79XX_PAD) * CONFIG_ADS79XX_CHANNELS));

	/* Set SSC at full speed for 8 channels.
	 * The clock (CLK) must rise when frame sync (FS) falls, the FS can be
	 * pulled high only afer 16 clocks have passed, samples are stored on
	 * the falling edge of the clock.
	 *
	 * FS len is used because the hardware needs at least one clock tick with
	 * FS high and we needed a bit of padding to result in a sampling freq
	 * that is an exact divisor of CPU_FREQ.
	 */
	SSC_CMR = MCK_DIV(CONFIG_ADS79XX_SAMPLE_RATE, CONFIG_ADS79XX_CHANNELS) & SSC_DIV_MASK;
	SSC_TCMR = SSC_CKS_DIV | SSC_CKO_CONT | SSC_CKG_NONE | DELAY | PERIOD | SSC_START_FALL_F;
	SSC_TFMR = DATALEN | DATNB | FSLEN | BV(SSC_MSBF) | SSC_FSOS_POSITIVE;

	// Receiver should start on TX and take the clock from TK
	SSC_RCMR = SSC_CKS_CLK | SSC_CKO_CONT | SSC_CKG_NONE | DELAY | PERIOD | SSC_START_TX;
	SSC_RFMR = DATALEN | DATNB | FSLEN | BV(SSC_MSBF) | SSC_FSOS_POSITIVE;

	PMC_PCER = BV(SSC_ID);
	SSC_CR = BV(SSC_TXEN) | BV(SSC_RXEN);

	// setup DMA interrupts
	SSC_IDR = 0xFFFFFFFF;

	AIC_SVR(SSC_ID) = ads_dmaIrqHandler;
	AIC_SMR(SSC_ID) = AIC_SRCTYPE_INT_EDGE_TRIGGERED | ADS_DMA_IRQ_PRIORITY;
	AIC_IECR = BV(SSC_ID);
	kprintf("SSC_SR [%08lX] SSC_IMR [%08lX]\n", SSC_SR, SSC_IMR);
#else
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

	DMA1->STR[DMA_STREAM].CR |= DMA_CR_CHAN(DMA_CHANNEL) | DMA_CR_DBUF | DMA_CR_PRI_MED |
	                            DMA_CR_MSIZE_16 | DMA_CR_PSIZE_16 | DMA_CR_MINC |
	                            DMA_CR_DIR_P2M | DMA_CR_TCIE;
	DMA1->STR[DMA_STREAM].PAR = (uint32_t)&SPI2->DR;
	/* clear interrupt enabled flag (just in case) */
	DMA_CLEAR_TCI(DMA1, DMA_STREAM);

	sysirq_setHandler(DMA_IRQ, ads_dmaIrqHandler);
#endif

	ctx->adc.set_ch = ads79xx_setChannel;
#if CPU_ARM_AT91
	ctx->adc.read = ads79xx_read;
#endif
	ctx->adc.stop = ads79xx_dmaStop;
#if 0
	ctx->adc.acquire = ads79xx_dmaAcquireBuf;
#endif
	ctx->adc.wait = ads79xx_wait;
	ctx->adc.start = ads79xx_dmaStart;
	DB(ctx->adc._type = ADC_ADS79XX);
}

#if CPU_ARM_AT91

/*
 * This IRQ only serves as an event generator, we don't need the data
 * acquired.
 */
static DECLARE_ISR(adstcp_dmaIrqHandler)
{
	AdcContext *ctx = &((Ads79xxTcp *)handle)->ads.adc;
	Ads79xxTcp *ctx_tcp = (Ads79xxTcp *)handle;
	uint8_t *tail = ctx->tail;
	uint8_t *tail_tcp = ctx_tcp->tail_tcp;
	uint8_t *sliceend = ctx->sliceend;

	size_t len;
	if (ctx->streaming_hook)
	{
		len = ctx->hook_slicelen;
		ctx->streaming_hook(ctx, tail_tcp, len);
	}
	else
		len = ctx->slicelen;

	MOVE_AND_WRAP(tail, len, ctx->base_buf, ctx->buf_len);
	MOVE_AND_WRAP(tail_tcp, len, ctx_tcp->tcp_buf, ctx_tcp->tcpbuf_len);

	uint8_t *mem = tail;
	MOVE_AND_WRAP(mem, len, ctx->base_buf, ctx->buf_len);

	/* set next dma transfer */
	SSC_RNPR = (reg32_t)mem;
	SSC_RNCR = SLICE_TO_SAMPLES(len);

	/* Signal only when the slicelen is ready */
	if (tail == sliceend)
	{
		MOVE_AND_WRAP(sliceend, ctx->slicelen, ctx->base_buf, ctx->buf_len);
		/* Update tail valid samples */
		cnt++;
		event_do(&((Ads79xx *)handle)->dma_done);
	}

	ctx->tail = tail;
	ctx_tcp->tail_tcp = tail_tcp;
	ctx->sliceend = sliceend;
	AIC_EOICR = 0;
}

static void fillTcpSlice(Ads79xxTcp *_ctx, void *_buf, size_t len)
{
	char *buf = (char *)_buf;

	if (_ctx->in_sock < 0)
	{
		// Check if there's a client trying to connect to us.
		// accept() doesn't block because server_sock is nonblocking
		_ctx->in_sock = lwip_accept(_ctx->server_sock, NULL, NULL);
		if (_ctx->in_sock == -1)
		{
			memset(buf, 0xFF, len);
			return;
		}
		//LOG_WARN("ADCTCP: Accepting incoming connection\n");
	}

	while (len)
	{
		ssize_t rd = lwip_recv(_ctx->in_sock, buf, len, 0);
		if (rd <= 0)
		{
			if (rd == 0)
				LOG_WARN("ADCTCP: Connection reset by peer\n");
			else
				LOG_ERR("ADCTCP: Broken socket\n");
			memset(buf, 0xFF, len);
			lwip_close(_ctx->in_sock);
			_ctx->in_sock = -1;
			return;
		}
		buf += rd;
		len -= rd;
	}
}

typedef struct SliceMsg
{
	Msg msg;
	uint16_t buf[ADC_TCP_SAMPLE_COUNT];
} SliceMsg;

	#define POOL_NUM     128
	#define ADC_TCP_PORT 30000

static MsgPort net_port;
static MsgPort adc_port;
static SliceMsg msg_pool[POOL_NUM];

    /* Make sure the following define always matches the number of pedals in the system! */
	#define PEDAL_NUM 6
static void copySamples(uint16_t *dst, const uint16_t *src, size_t samples_to_copy)
{
	ASSERT(samples_to_copy % PEDAL_NUM == 0);
	for (size_t i = 0; i < samples_to_copy / PEDAL_NUM; ++i)
	{
		for (int j = 0; j < SAMPLE_RATE_RATIO; ++j)
		{
			memcpy(dst, src, sizeof(uint16_t) * PEDAL_NUM);
			dst += PEDAL_NUM;
		}
		src += PEDAL_NUM;
	}
}

static void *ads79xxtcp_wait(AdcContext *ctx)
{
	Ads79xxTcp *_ctx = ADS79XXTCP_CAST(ctx);
	uint8_t *ret_buf;

	switch (_ctx->ads.mode)
	{
	case ADC_STREAMING:
		ASSERT(cnt >= 0);

		// Prefill the next tcp block
		uint8_t *next_head_tcp = _ctx->head_tcp;
		MOVE_AND_WRAP(next_head_tcp, ctx->slicelen, _ctx->tcp_buf, _ctx->tcpbuf_len);

		SliceMsg *msg = (SliceMsg *)msg_get(&adc_port);
		ASSERT(msg != NULL);
		copySamples((uint16_t *)next_head_tcp, msg->buf, ADC_TCP_SAMPLE_COUNT);
		msg_reply(&msg->msg);
		// Wait for an event (could be already arrived if we were slow)
		event_wait(&_ctx->ads.dma_done);

		// TODO: if this assert fails, the condition has to be handled!
		// (time stamp in reduction should be adjusted)
		int ocnt = cnt;
		cnt = 0;
		ASSERT(ocnt >= 0);
		ASSERT(ocnt != 0);
		if (ocnt > 1)
			LOG_WARN("ADC: buffer overrun (streaming hook might be missing data)\n");

		ret_buf = _ctx->head_tcp;
		/* move head forward */
		MOVE_AND_WRAP(ctx->head, ctx->slicelen, ctx->base_buf, ctx->buf_len);
		_ctx->head_tcp = next_head_tcp;
		break;
	default:
		ASSERT(0);
		ret_buf = NULL;
		break;
	}
	return ret_buf;
}

static NORETURN void network_proc(void)
{
	ADC_TCP_STROBE_INIT();
	ADC_TCP_STROBE_LOW();
	Ads79xxTcp *ctx = (Ads79xxTcp *)proc_currentUserData();

	int server_sock = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock < 0)
		LOG_ERR("Error creating ADC TCP server socket\n");

	// Set the server socket in non-blocking mode, so that the accept()
	// won't block.
	uint32_t nonblocking = 1;
	lwip_ioctl(server_sock, FIONBIO, &nonblocking);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ADC_TCP_PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (lwip_bind(server_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		LOG_ERR("Error binding port %hd\n", ADC_TCP_PORT);
	if (lwip_listen(server_sock, 0) == -1)
		LOG_ERR("Error listening socket\n");

	ctx->server_sock = server_sock;
	ctx->in_sock = -1;

	msg_initPort(&net_port, event_createGeneric());
	for (int i = 0; i < POOL_NUM; i++)
		msg_put(&net_port, &msg_pool[i].msg);

	while (1)
	{
		SliceMsg *msg;
		ADC_TCP_STROBE_HIGH();
		while ((msg = (SliceMsg *)msg_get(&net_port)))
		{
			fillTcpSlice(ctx, msg->buf, ADC_TCP_SAMPLE_COUNT * sizeof(uint16_t));
			msg->msg.replyPort = &net_port;
			msg_put(&adc_port, &msg->msg);
		}
		ADC_TCP_STROBE_LOW();
		event_wait(&net_port.event);
	}
}

static void ads79xxtcp_dmaStart(AdcContext *ctx)
{
	Ads79xxTcp *_ctx = ADS79XXTCP_CAST(ctx);
	ASSERT(_ctx->tcpbuf_len == ctx->buf_len);
	ASSERT(_ctx->tcp_buf != ctx->base_buf);
	_ctx->tail_tcp = _ctx->head_tcp = _ctx->tcp_buf;

	// Read immediately the first TCP block. This is needed so that the
	// streaming hook will then process it at the right frequency as if
	// the data were flowing through.
	event_wait(&adc_port.event);
	SliceMsg *msg = (SliceMsg *)msg_get(&adc_port);
	ASSERT(msg != NULL);
	ASSERT(ctx->slicelen == ADC_TCP_SAMPLE_COUNT * SAMPLE_RATE_RATIO * sizeof(uint16_t));
	copySamples((uint16_t *)_ctx->head_tcp, msg->buf, ADC_TCP_SAMPLE_COUNT);
	msg_reply(&msg->msg);
	// wait another slice otherwise dmaWait() will fail
	event_wait(&adc_port.event);

	ads79xx_dmaStart(ctx);
}

void ads79xxtcp_init(Ads79xxTcp *ctx, Ads79xxRange range, uint8_t *tcp_buf, size_t tcpbuf_len)
{
	ads79xx_init(&ctx->ads, range);
	AIC_SVR(SSC_ID) = adstcp_dmaIrqHandler;
	ctx->ads.adc.wait = ads79xxtcp_wait;
	ctx->ads.adc.start = ads79xxtcp_dmaStart;

	ctx->tcp_buf = tcp_buf;
	ctx->tcpbuf_len = tcpbuf_len;

	msg_initPort(&adc_port, event_createGeneric());
	Process *p = proc_new(network_proc, ctx, KERN_MINSTACKSIZE * 5, NULL);
	ASSERT(p);
	proc_setPri(p, 10);
}

#endif
