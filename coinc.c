
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