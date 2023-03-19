
/*
    Copyright (C) 2013-2020 Jaakko Julin <jaakko.julin@jyu.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See file COPYING for details.
*/



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <coinc_config.h>

#define N_ADCS_MAX 128
#define COINC_TABLE_SIZE_DEFAULT 20
#define N_ADCS_DEFAULT 8
#define SKIP_LINES_DEFAULT 0
#define TIMING_WINDOW_HIGH_DEFAULT 0
#define TIMING_WINDOW_LOW_DEFAULT 0
#define TRIGGER_ADC_DEFAULT 0
#define MIN_MULTIPLICITY_DEFAULT 2
#define HELP_TEXT "Usage: %s [OPTION] infile outfile\n\nIf no infile or outfile is specified, standard input or output is used respectively.\nValid options:\n\t--timestamps\toutput timestamps\n\t--both\t\toutput both data and timestamps (2 col/ch)\n\t--timediff\toutput both data and time difference to trigger time\n\t--nadc=NUM\tprocess a maximum of NUM ADCs\n\t--skip=NUM\tskip first NUM lines from the beginning of the input\n\t--tablesize=NUM\tuse a coincidence table of NUM events\n\t--nevents=NUM\toutput maximum of NUM events\n\t--trigger=NUM\tuse ADC NUM as the triggering ADC\n\t--verbose\tverbose output\n\t--low=ADC,NUM\tset timing window for ADC low (NUM ticks)\n\t--high=ADC,NUM\tset timing window for ADC high (NUM ticks)\n\t--multiplicity=NUM\tminimum of NUM channels per coincidence\n\t--require=ADC\tcoincidence must include ADC\n\t--triggertime\tinclude trigger event timestamp as first column\n\n"
#define  LICENCE_TEXT "This program is free software; you can redistribute it and/or modify\nit under the terms of the GNU General Public License as published by\nthe Free Software Foundation; either version 2 of the License, or\n(at your option) any later version.\n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.\n"

int verbose=0;
int silent=0;

struct list_event {
    int adc;
    int channel;
    unsigned long long int timestamp;
};

typedef struct list_event event;

typedef enum OUTPUT_MODE_E {
	MODE_RAW = 0,
    MODE_TIMESTAMPS = 1,
    MODE_TIME_AND_CHANNEL = 2,
    MODE_TIMEDIFF_AND_CHANNEL = 3
} output_mode;

struct monitor {
   FILE *f;
   event last_event;
   unsigned int sum;
};

typedef struct monitor monitor_t;

void insert_blank_event(event *event) {
    event->adc=N_ADCS_MAX-1;
    event->channel=-1;
    event->timestamp=0;
}

monitor_t *init_monitor(char *filename) {
    monitor_t *mon=malloc(sizeof(monitor_t));
    mon->f=fopen(filename, "r");
    if(!mon->f) {
        free(mon);
        return NULL;
    }
    mon->last_event.adc=0;
    mon->last_event.channel=0;
    mon->last_event.timestamp=0;
    mon->sum=0;
    return mon;
}

int advance_monitorfile_until_timestamp(monitor_t *mon, unsigned long long int ts) {
    while(ts > mon->last_event.timestamp) { /* This will actually go one too far (how else are we going to know when to stop? */
        if(fscanf(mon->f,"%i %i %llu\n",&(mon->last_event.adc), &(mon->last_event.channel), &(mon->last_event.timestamp)) == 3) {
            mon->sum++;
        } else {
            return (mon->sum-1);
        }
    }
    return (mon->sum-1);
}


int read_event_from_file(FILE *file, event *event, int n_adcs) {
    if(fscanf(file,"%i %i %llu\n",&event->adc, &event->channel, &event->timestamp) == 3) {
        if(event->adc < n_adcs && event->adc>=0) {
            return 1;
        } else {
            fprintf(stderr, "ADC value %u too high or negative, aborting. Check input file or try increasing number of ADCs (currently %i).\n", event->adc, n_adcs);
            return 0;
        }

    } else {
		if(!feof(file)) {
			fprintf(stderr, "\nError in input data.\n");
		}
		return 0;
	}
}

int find_percentile(double percentile, unsigned int *histogram, int low, int high) {
    unsigned int integral=0;
    int i, i_max=high-low;
    unsigned int stop=0;
    for(i=0; i<=i_max; i++) {
        integral += histogram[i];
    }
    stop=(unsigned int) (percentile*integral*1.0);
    integral=0;
    for(i=0; i<=i_max; i++) {
        integral += histogram[i];
        if(integral >= stop)
            return (i+low);
    }
    return (low-1);