#ifndef UART_AGREEMENT_C_API_H
#define UART_AGREEMENT_C_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*uart_agreement_rx_callback_t)(uint8_t cmd,
                                             const uint8_t *data,
                                             uint8_t len,
                                             void *user_data);

int uart_agreement_register_callback(uart_agreement_rx_callback_t cb, void *user_data);
int uart_agreement_init(void);
int uart_agreement_start(void);
int uart_agreement_stop(void);

void uart_agreement_internal_on_frame(uint8_t cmd, const uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif
