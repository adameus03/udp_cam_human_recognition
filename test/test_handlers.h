#include "def.h"

int registration_sock_handler(int client_sock);
int unregistration_sock_handler(int client_sock);
int initcomm_sock_handler(int client_sock);
int start_unconditional_stream_sock_handler(int client_sock);
int stop_unconditional_stream_sock_handler(int client_sock);
int nop_sock_handler(int client_sock);

int logs_mode_minimal_sock_handler(int client_sock);
int logs_mode_complete_sock_handler(int client_sock);

int energy_saving_shutdown_analyser_sock_handler(int client_sock);
int energy_saving_wakeup_analyser_sock_handler(int client_sock);