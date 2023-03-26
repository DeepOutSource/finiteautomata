
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
}

int main (int argc, char **argv) {
    unsigned int i=0;
    unsigned int j=0,k=0;
    unsigned int coinc_table_size=COINC_TABLE_SIZE_DEFAULT, coinc_table_size_argument;
    unsigned int coincs_found=0;
    unsigned int lines_read=0;
    int trigger_adc=TRIGGER_ADC_DEFAULT,trigger_adc_argument;
	unsigned int n_adcs_argument=0,n_adcs=N_ADCS_DEFAULT;
	int require_argument;
    int adc;
	unsigned int adcs_in_coinc;
	output_mode output_mode=MODE_RAW;
    int triggertime=0;
	int *coinc_events, *n_adc_events, *n_coinc_adc_events;
	long long int time_difference;
    long long int *time_window_high=malloc(N_ADCS_MAX*sizeof(long long int));
	long long int *time_window_low=malloc(N_ADCS_MAX*sizeof(long long int));
    long long int time_window_argument=0;
    int *require = (int *)malloc(N_ADCS_MAX*(sizeof(int)));
    int all_required_found;
    int min_multiplicity=MIN_MULTIPLICITY_DEFAULT;
    int adc_argument=0;
	int endgame=0;
	int skip_lines_argument=0,skip_lines=SKIP_LINES_DEFAULT;
    int output_n_events=0;
    monitor_t *monitor=NULL;
    char *monitorfilename=calloc(256, sizeof(char));
	char buffer[100];
	event *coinc_table;

    FILE *read_file=stdin;
	FILE *output_file=stdout;
    unsigned int **timediff_histogram;


    if(argc==1) {
        fprintf(stderr, "coinc %s\n", coinc_VERSION);
		fprintf(stderr, HELP_TEXT, argv[0]);
    	fprintf(stderr, LICENCE_TEXT);
		return 0;
	}
    for(i=0; i<N_ADCS_MAX; i++) {
        time_window_low[i]=TIMING_WINDOW_LOW_DEFAULT;
        time_window_high[i]=TIMING_WINDOW_HIGH_DEFAULT;
        require[i]=0;
    }
	for(i=1; i<(unsigned int)argc; i++) {
		if(verbose) fprintf(stderr, "Scanning argument no %i/%i (\"%s\")...\n", i, argc-1, argv[i]);
		if(strcmp(argv[i], "--verbose")==0) {
			fprintf(stderr, "Verbose output mode active.\n");
			verbose=1;
			continue;
		}
        if(strcmp(argv[i], "--timestamps")==0) {
            output_mode=MODE_TIMESTAMPS;
            if(verbose) fprintf(stderr, "Outputting timestamp values.\n");
            continue;
        }
        if(strcmp(argv[i], "--triggertime")==0) {
            triggertime=1;
            if(verbose) fprintf(stderr, "Outputting also trigger timestamp (first column)\n");
            continue;
        }
        if(sscanf(argv[i], "--monitor=%200s", monitorfilename)==1) {
            if(verbose) fprintf(stderr, "Using file %s for monitor data\n", monitorfilename);
            monitor=init_monitor(monitorfilename);
            if(!monitor) {
                fprintf(stderr, "Could not open file %s! Monitor data ignored\n", monitorfilename);
            }
            continue;
        }

        if(strcmp(argv[i], "--silent")==0) {
            silent=1;
            continue;
        }
        if(strcmp(argv[i], "--both")==0) {
            output_mode=MODE_TIME_AND_CHANNEL;
            if(verbose) fprintf(stderr, "Outputting both channel and timestamp values.\n");
            continue;
        }
        if(strcmp(argv[i], "--timediff")==0) {
            output_mode=MODE_TIMEDIFF_AND_CHANNEL;
            if(verbose) fprintf(stderr, "Outputting both channel and time diff to trigger time.\n");
            continue;
        }

		if(sscanf(argv[i], "--skip=%u", &skip_lines_argument)==1) {
			skip_lines=skip_lines_argument;
			if(verbose) fprintf(stderr, "Skipping first %u lines of input file...\n", skip_lines); 
			continue;
		}
		if(sscanf(argv[i], "--nadc=%u", &n_adcs_argument)==1) {
			if(n_adcs_argument > 1 && n_adcs_argument < N_ADCS_MAX-1) {
				if (verbose) {
					fprintf(stderr, "Number of ADCs set to be %u\n", n_adcs_argument);
				}
				n_adcs=n_adcs_argument;
			} else {
				fprintf(stderr, "Number of ADCs must be higher than 1 but lower than %i!\n", N_ADCS_MAX-1);
				return 0;
			}
			continue;
		}
		if(sscanf(argv[i], "--tablesize=%u", &coinc_table_size_argument)==1) {
			if(coinc_table_size_argument>1) {
				coinc_table_size=coinc_table_size_argument;
				if(verbose) {
					fprintf(stderr, "Coinc table size set to be %u\n", coinc_table_size_argument);
				}
			} else {
				fprintf(stderr, "Coinc table size must be larger than 1!\n");
				return 0;
			}
            continue;
		}
		
		if(sscanf(argv[i], "--trigger=%u", &trigger_adc_argument)==1) {
			trigger_adc=trigger_adc_argument;
            continue;
		}
        
        if(sscanf(argv[i],"--low=%i,%lli", &adc_argument, &time_window_argument)==2) {
            if(verbose) {
                fprintf(stderr, "Set low value %lli for adc %i\n", time_window_argument, adc_argument);
            }
            if(adc_argument < 0 || adc_argument >= n_adcs) {
                fprintf(stderr, "ADC number %i too high or negative (in --low=%i,%lli)\n", adc_argument, adc_argument, time_window_argument);
            }
			time_window_low[adc_argument]=time_window_argument;
            continue;
		}
        if(sscanf(argv[i], "--low=%lli", &time_window_argument)==1) {
            if(verbose) {