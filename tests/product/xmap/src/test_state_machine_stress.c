/*
 * This code defines a state-machine for an xMAP from xiaStartSystem
 * -> xiaExit.  Based on the allowed set of transitions, it generates
 * one for the program to execute.
 *
 * $Id$
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "xia_common.h"

#include "handel.h"
#include "handel_errors.h"

enum states {
    STATE_ANY,
    STATE_ERROR,
    STATE_START,
    STATE_END,
    STATE_MAPPING,
    STATE_MCA,
    STATE_MAPPING_RUN,
    STATE_BUFFER_A,
    STATE_BUFFER_B,
    STATE_MCA_RUN,
    /* Sentinel to mark end of list. Not an actual state. */
    STATE_EOL
};

static char* STATES[] = {"any",      "error",   "start",       "end",
                         "mapping",  "mca",     "mapping run", "buffer a",
                         "buffer b", "mca run", "eol"};

enum events {
    EVT_ANY,
    EVT_SWITCH_TO_MAPPING,
    EVT_SWITCH_TO_MCA,
    EVT_START_MCA_RUN,
    EVT_START_MAPPING_RUN,
    EVT_WAIT_FOR_BUFFER_A,
    EVT_WAIT_FOR_BUFFER_B,
    EVT_STOP_RUN,
    EVT_READ_MCA,
    /* Sentinel to mark end of list. Not an actual event. */
    EVT_EOL
};

static char* EVENTS[] = {"any",
                         "switch to mapping",
                         "switch to mca",
                         "start mca run",
                         "starting mapping run",
                         "wait for buffer a",
                         "wait for buffer b",
                         "stop run",
                         "read mca",
                         "eol"};

struct transition {
    enum states st;
    enum events evt;
    enum states (*fn)(void);
};

static enum states switch_to_mapping(void);
static enum states switch_to_mca(void);
static enum states invalid(void);
static enum states shutdown(void);
static enum states start_mapping_run(void);
static enum states do_buffer_a(void);
static enum states do_buffer_b(void);
static enum states stop_run(void);
static enum states read_mca(void);
static enum states start_mca_run(void);

static enum events next_event(void);

static struct transition TRANSITIONS[] = {
    {STATE_START, EVT_SWITCH_TO_MAPPING, switch_to_mapping},
    {STATE_START, EVT_SWITCH_TO_MCA, switch_to_mca},
    {STATE_MAPPING, EVT_SWITCH_TO_MCA, switch_to_mca},
    {STATE_MAPPING, EVT_START_MAPPING_RUN, start_mapping_run},
    {STATE_MAPPING_RUN, EVT_WAIT_FOR_BUFFER_A, do_buffer_a},
    {STATE_MAPPING_RUN, EVT_STOP_RUN, stop_run},
    {STATE_BUFFER_A, EVT_WAIT_FOR_BUFFER_B, do_buffer_b},
    {STATE_BUFFER_A, EVT_STOP_RUN, stop_run},
    {STATE_BUFFER_B, EVT_WAIT_FOR_BUFFER_A, do_buffer_a},
    {STATE_BUFFER_B, EVT_STOP_RUN, stop_run},
    {STATE_MCA, EVT_START_MCA_RUN, start_mca_run},
    {STATE_MCA, EVT_SWITCH_TO_MAPPING, switch_to_mapping},
    {STATE_MCA_RUN, EVT_READ_MCA, read_mca},
    {STATE_MCA_RUN, EVT_STOP_RUN, stop_run},
    {STATE_ERROR, EVT_ANY, shutdown},
    {STATE_ANY, EVT_ANY, invalid}};

/* We use this since our notion of "invalid" just means, "try another
 * event, please".
 */
static enum states PREV_STATE;

int main(int argc, char* argv[]) {
    int i;
    int status;
    int n_trans = sizeof(TRANSITIONS) / sizeof(*TRANSITIONS);

    enum states state = STATE_START;

    FILE* actions;

    UNUSED(argc);

    srand((unsigned) time(NULL));

    xiaSetLogLevel(4);
    xiaSetLogOutput("handel.log");

    status = xiaInit(argv[1]);

    if (status != XIA_SUCCESS) {
        exit(1);
    }

    status = xiaStartSystem();

    if (status != XIA_SUCCESS) {
        exit(1);
    }

    actions = fopen("actions.log", "w");
    assert(actions);

    while (state != STATE_END) {
        enum events e;

        PREV_STATE = state;

        e = next_event();

        for (i = 0; i < n_trans; i++) {
            if ((state == TRANSITIONS[i].st) || (STATE_ANY == TRANSITIONS[i].st)) {
                if ((e == TRANSITIONS[i].evt) || (EVT_ANY == TRANSITIONS[i].evt)) {
                    /* Don't print out invalid ones. */
                    if (TRANSITIONS[i].fn != invalid) {
                        fprintf(actions, "%d [%s], %d [%s]\n", state, STATES[state], e,
                                EVENTS[e]);
                        fflush(actions);
                    }

                    state = (TRANSITIONS[i].fn)();
                    break;
                }
            }
        }
    }

    xiaExit();

    fclose(actions);

    return 0;
}

static enum states switch_to_mapping(void) {
    int status;
    int ignore = 0;

    double map_mode = 3.0;
    double list_variant = 2.0;

    status = xiaSetAcquisitionValues(0, "mapping_mode", &map_mode);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    status = xiaSetAcquisitionValues(0, "list_mode_variant", &list_variant);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    status = xiaBoardOperation(0, "apply", &ignore);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_MAPPING;
}

static enum states switch_to_mca(void) {
    int status;
    int ignore = 0;

    double mca_mode = 0.0;

    status = xiaSetAcquisitionValues(0, "mapping_mode", &mca_mode);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    status = xiaBoardOperation(0, "apply", &ignore);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_MCA;
}

static enum states invalid(void) {
    return PREV_STATE;
}

static enum states shutdown(void) {
    int status;

    fprintf(stderr, "Error reported, shutting down.\n");

    status = xiaStopRun(-1);

    return STATE_END;
}

static enum events next_event(void) {
    int r;
    int n_events = EVT_EOL;

    r = (int) ROUND(((double) rand() / (double) RAND_MAX) * (n_events - 2));
    r++;

    assert(r >= 1 && r < n_events);

    return r;
}

static enum states start_mapping_run(void) {
    int status;

    status = xiaStartRun(-1, 0);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_MAPPING_RUN;
}

static enum states do_buffer_a(void) {
    int status;

    unsigned short full = 0;

    unsigned long len;

    unsigned long* buffer;

    char buffer_done = 'a';

    do {
        status = xiaGetRunData(0, "buffer_full_a", &full);

        if (status != XIA_SUCCESS) {
            return STATE_ERROR;
        }
    } while (!full);

    status = xiaGetRunData(0, "list_buffer_len_a", &len);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    buffer = malloc(len * sizeof(unsigned long));
    assert(buffer);

    status = xiaGetRunData(0, "buffer_a", buffer);

    free(buffer);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    status = xiaBoardOperation(0, "buffer_done", &buffer_done);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_BUFFER_A;
}

static enum states do_buffer_b(void) {
    int status;

    unsigned short full = 0;

    unsigned long len;

    unsigned long* buffer;

    char buffer_done = 'b';

    do {
        status = xiaGetRunData(0, "buffer_full_b", &full);

        if (status != XIA_SUCCESS) {
            return STATE_ERROR;
        }
    } while (!full);

    status = xiaGetRunData(0, "list_buffer_len_b", &len);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    buffer = malloc(len * sizeof(unsigned long));
    assert(buffer);

    status = xiaGetRunData(0, "buffer_b", buffer);

    free(buffer);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    status = xiaBoardOperation(0, "buffer_done", &buffer_done);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_BUFFER_B;
}

static enum states stop_run(void) {
    int status;

    status = xiaStopRun(-1);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_START;
}

static enum states read_mca(void) {
    int status;

    unsigned long* buffer;

    unsigned long len;

    status = xiaGetRunData(0, "mca_length", &len);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    /* Use the module_mca */

    buffer = malloc(len * 4 * sizeof(unsigned long));
    assert(buffer);

    status = xiaGetRunData(0, "module_mca", buffer);

    free(buffer);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_MCA_RUN;
}

static enum states start_mca_run(void) {
    int status;

    status = xiaStartRun(-1, 0);

    if (status != XIA_SUCCESS) {
        return STATE_ERROR;
    }

    return STATE_MCA_RUN;
}
