#ifndef CFG_BTN_H_
#define CFG_BTN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Callback type for button press. timeout_ms will reflect the matched duration threshold */
typedef void (*cfg_btn_callback_t)(uint32_t timeout_ms);

/* Initialize the button handler thread and register the user callback */
void cfg_btn_init(cfg_btn_callback_t cb);

/* To be called from EXTI ISR */
void cfg_btn_handle_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* CFG_BTN_H_ */
