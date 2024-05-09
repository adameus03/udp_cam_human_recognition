#include "test_handlers.h"

#define MAX_COMMAND_SIZE 100

int get_user_command(char *command, int max_command_size) {
    printf("Enter a command: ");
    fgets(command, max_command_size, stdin);
    return 0;
}

int client_sock_handler(int client_sock) {
    
    while (1) {
        // Get user command
        char command[MAX_COMMAND_SIZE];
        if (0 != get_user_command(command, MAX_COMMAND_SIZE)) {
            return 1;
        }

        if (0 == strcmp(command, "register\n")) {
            if (0 != registration_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "unregister\n")) {
            if (0 != unregistration_sock_handler(client_sock)) {
                return 1;
            }
        }
        else if (0 == strcmp(command, "initcomm\n")) {
            if (0 != initcomm_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "start\n")) {
            if (0 != start_unconditional_stream_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "stop\n")) {
            if (0 != stop_unconditional_stream_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "nop\n")) {
            if (0 != nop_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "logs minimal\n")) {
            if (0 != logs_mode_minimal_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "logs complete\n")) {
            if (0 != logs_mode_complete_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "ana shut\n")) {
            if (0 != energy_saving_shutdown_analyser_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "ana wake\n")) {
            if (0 != energy_saving_wakeup_analyser_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "dev reset\n")) {
            if (0 != software_device_reset_sock_handler(client_sock)) {
                return 1;
            }
        } else if (0 == strcmp(command, "exit\n")) { // Not really a server command, but enables user to gracefully exit the program
            break;
        } else {
            printf("Invalid command\n");
        }
    }
    return 0;

}