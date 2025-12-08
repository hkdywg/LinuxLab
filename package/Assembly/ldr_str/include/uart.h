#ifndef __MINI_UART_H_
#define __MINI_UART_H_

void uart_init(void);
char uart_recv(void);
void uart_send(char c);
void uart_send_string(char *str);

#endif
