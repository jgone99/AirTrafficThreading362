// Module to implement the ground controller application-layer protocol.

// The protocol is fully defined in the README file. This module
// includes functions to parse and perform commands sent by an
// airplane (the docommand function), and has functions to send
// responses to ensure proper and consistent formatting of these
// messages.

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#include "util.h"
#include "airplane.h"
#include "airs_protocol.h"
#include "airplanelist_control.h"
#include "queue_manager.h"

/************************************************************************
 * Call this response function if a command was accepted
 */
void send_ok(airplane *plane) {
    fprintf(plane->fp_send, "OK\n");
}

/************************************************************************
 * Call this response function if an error can be described by a simple
 * string.
 */
void send_err(airplane *plane, char *desc) {
    fprintf(plane->fp_send, "ERR %s\n", desc);
}

/************************************************************************
 * Call this response function if you want to embed a specific string
 * argument (sarg) into an error reply (which is now a format string).
 */
void send_err_sarg(airplane *plane, char *fmtstring, char *sarg) {
    fprintf(plane->fp_send, "ERR ");
    fprintf(plane->fp_send, fmtstring, sarg);
    fprintf(plane->fp_send, "\n");
}

/************************************************************************
 * Handle the "REG" command.
 */
static void cmd_reg(airplane *plane, char *rest) {
    if(plane->state != PLANE_UNREG) {
        send_err_sarg(plane, "Already registered as %s", plane->id);
        return;
    }
    
    if(rest == NULL) {
        send_err(plane, "REG missing flightid");
        return;
    }

    if(strlen(rest) > PLANE_MAXID) {
        send_err(plane, "Invalid flight id -- too long");
        return;
    }
    
    char *c = rest;
    for(; isalnum(*c) > 0; c++); // :)

    if(*c != '\0') {
        send_err(plane, "Invalid flight id -- only alphanumeric characters allowed");
        return;
    }
    
    if(planelist_containsid(rest) == 1) {
        send_err(plane, "flightid already in use -- register with different flightid");
        return;
    }

    planelist_add(plane, rest);
    send_ok(plane);
    plane->state = PLANE_ATTERMINAL;   
}

/************************************************************************
 * Handle the "REQTAXI" command.
 */
static void cmd_reqtaxi(airplane *plane, char *rest) {
    if(plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }

    if(plane->state != PLANE_ATTERMINAL) {
        send_err(plane, "Plane not at terminal -- cannot process request");
        return;
    }
    
    plane->state = PLANE_TAXIING;
    send_ok(plane);
    planequeue_add(plane->id);
    
}

/************************************************************************
 * Handle the "REQPOS" command.
 */
static void cmd_reqpos(airplane *plane, char *rest) {
    if(plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }
    
    if(plane->state != PLANE_TAXIING) {
        send_err(plane, "Plane not taxxing -- cannot process request");
    }

    
}

/************************************************************************
 * Handle the "REQAHEAD" command.
 */
static void cmd_reqahead(airplane *plane, char *rest) {
    if(plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }
    send_err(plane, "REQAHEAD command not yet implemented");
}

/************************************************************************
 * Handle the "INAIR" command.
 */
static void cmd_inair(airplane *plane, char *rest) {
    if(plane->state == PLANE_UNREG) {
        send_err(plane, "Unregistered plane -- cannot process request");
        return;
    }
    
    if(plane->state != PLANE_CLEAR) {
        send_err(plane, "Plane not cleared -- cannot process request");
        return;
    }
    plane->state = PLANE_INAIR;
    send_ok(plane);
    planequeue_inair();
    
}

/************************************************************************
 * Handle the "BYE" command.
 */
static void cmd_bye(airplane *plane, char *rest) {
    plane->state = PLANE_DONE;
}

/************************************************************************
 * Parses and performs the actions in the line of text (command and
 * optionally arguments) passed in as "command".
 */
void docommand(airplane *plane, char *command) {
    char *saveptr;
    char *cmd = strtok_r(command, " \t\r\n", &saveptr);

    if (cmd == NULL) {  // Empty line (no command) -- just ignore line
        return;
    }

    // Get arguments (everything after command, trimmed)
    char *args = strtok_r(NULL, "\r\n", &saveptr);
    if (args != NULL) {
        args = trim(args);
    }

    // TODO: Only some commands are recognized below. Must include all
    if (strcmp(cmd, "REG") == 0) {
        cmd_reg(plane, args);
    } else if (strcmp(cmd, "REQTAXI") == 0) {
        cmd_reqtaxi(plane, args);
    } else if(strcmp(cmd, "REQPOS") == 0) {
        cmd_reqpos(plane, args);
    } else if(strcmp(cmd, "REQAHEAD") == 0) {
        cmd_reqahead(plane, args);
    } else if(strcmp(cmd, "INAIR") == 0) {
        cmd_inair(plane, args);
    } else if (strcmp(cmd, "BYE") == 0) {
        cmd_bye(plane, args);
    } else {
        send_err(plane, "Unknown command");
    }
}
