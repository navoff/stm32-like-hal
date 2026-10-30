/**
 * @file unit_tests.c
 * @brief UART driver unit tests: ORE error provocation and Tx stream test.
 */

#include "config.h"

#include <stdlib.h>
#include <time.h>

#include "drivers/tick_timer.h"

#include "utils/unit_tests.h"

#if (UNIT_TEST_ERROR_ORE)
  /// An ORE error is provoked every UNIT_TEST_UART_ORE_BYTE_NUMBER received bytes
  #define UNIT_TEST_UART_ORE_BYTE_NUMBER 4

  #define UNIT_TEST_UART_ORE_REPETITION 10

/// Structure for checking that the UART OverrunError (ORE) is handled correctly
typedef struct
{
  uint8_t repetition;

  uint16_t rx_byte_counter;

  uint8_t causes_counter;
  uint8_t error_counter;

} unit_test_uart_ore_t;
#endif

#if (UNIT_TEST_UART_TX)
  /// Maximum interval between data transmissions, [ms]
  #define UNIT_TEST_RS485_MAX_TX_INTERVAL 15

  /// Maximum number of bytes sent per period
  #define UNIT_TEST_RS485_BYTES_PER_PERIOD 10

/// Structure for testing UART transmission
typedef struct
{
  /// Transmission period
  uint8_t period;
  /// Number of bytes sent per period
  uint8_t number_bytes;

  /// Timestamp for tracking the period
  uint32_t timestamp;

  /// Pointer to the transmit function
  unit_test_uart_tx_byte callback;
} unit_test_uart_tx_t;
#endif

#if (UNIT_TEST_ERROR_ORE)
unit_test_uart_ore_t unit_test_uart_ore = {
    .repetition = UNIT_TEST_UART_ORE_REPETITION,
    .rx_byte_counter = 0,
    .causes_counter = 0,
    .error_counter = 0,
};
#endif

#if (UNIT_TEST_UART_TX)
unit_test_uart_tx_t unit_test_uart_tx = {
    .period = 1,
    .number_bytes = 1,
    .callback = NULL,
};
#endif

#if (UNIT_TEST_ERROR_ORE)
/**
 * @brief Function that provokes an ORE error
 * @note Must be called right before UARTx->DR is read
 */
void unit_test_uart_ore_cause(void)
{
  unit_test_uart_ore.rx_byte_counter++;

  if ((unit_test_uart_ore.rx_byte_counter % UNIT_TEST_UART_ORE_BYTE_NUMBER) == 0)
  {
    if (unit_test_uart_ore.causes_counter < unit_test_uart_ore.repetition)
    {
      unit_test_uart_ore.causes_counter++;
      for (int i = 0; i < 1000000; i++)
      {
        __NOP();
        __NOP();
        __NOP();
      }
    }
  }
}

/**
 * @brief Function that checks the ORE error is handled correctly
 * @note The function must be called before any possible calls of the error handler.
 *        It must be passed the same SR value as the error handler
 * @param sr - state of the UARTx->SR register
 */
void unit_test_uart_ore_process(uint16_t sr)
{
  if (sr & USART_SR_ORE)
    unit_test_uart_ore.error_counter++;

  if (unit_test_uart_ore.error_counter != unit_test_uart_ore.causes_counter)
  {
    // The test must produce as many ORE errors as there were provocations of this error!
    hal_assert(false);
  }
}
#endif

#if (UNIT_TEST_UART_TX)

/**
 * @brief Initialize the data transmission test
 * @param func - pointer to the function that sends 1 byte
 */
void unit_test_uart_tx_init(unit_test_uart_tx_byte func)
{
  if (func == NULL)
  {
    hal_assert(false);
  }

  unit_test_uart_tx.callback = func;

  srand(time(NULL));
  START_TIMER(unit_test_uart_tx.timestamp);
}

/**
 * @brief Data transmission process
 */
void unit_test_uart_tx_procress(void)
{
  static uint8_t data = 0;

  if (CHECK_TIMER(unit_test_uart_tx.timestamp, unit_test_uart_tx.period))
  {
    START_TIMER(unit_test_uart_tx.timestamp);

    unit_test_uart_tx.period = rand() % UNIT_TEST_RS485_MAX_TX_INTERVAL + 1;
    unit_test_uart_tx.number_bytes = rand() % UNIT_TEST_RS485_BYTES_PER_PERIOD;

    for (int i = 0; i < unit_test_uart_tx.number_bytes; i++)
    {
      if (unit_test_uart_tx.callback(data++) != ERR_OK)
      {
        hal_assert(false);
      }
    }
  }
}
#endif
