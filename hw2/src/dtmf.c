#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "const.h"
#include "audio.h"
#include "dtmf.h"
#include "dtmf_static.h"
#include "goertzel.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int get_row_frequency(char symbol) {
	for(int i = 0; i < NUM_DTMF_ROW_FREQS; i++) {
		for (int j = 0; j < NUM_DTMF_COL_FREQS; j++) {
			if (*(*(dtmf_symbol_names+i)+j) == symbol) {
				if (i == 0) {
					return 697;
				}
				if (i == 1) {
					return 770;
				}
				if (i == 2) {
					return 852;
				}
				if (i == 3) {
					return 941;
				}
			}
		}
	}

	return -1;
}

int get_col_frequency(char symbol) {
	for(int i = 0; i < NUM_DTMF_ROW_FREQS; i++) {
		for (int j = 0; j < NUM_DTMF_COL_FREQS; j++) {
			if (*(*(dtmf_symbol_names+i)+j) == symbol) {
				if (j == 0) {
					return 1209;
				}
				if (j == 1) {
					return 1336;
				}
				if (j == 2) {
					return 1477;
				}
				if (j == 3) {
					return 1633;
				}
			}
		}
	}
	return -1;
}

int char_to_int(char c) {
	int num = c - '0';
	if (num < 0 || num > 9) {
		return -1;
	} else {
		return num;
	}
}

int dtmf_generate_helper(int num_untrunc, FILE* out) {
	return audio_write_sample(out, num_untrunc);
}

double get_w() {
	double z = pow(10, noise_level / 10.0);
	// printf("%d\n", noise_level);
	// printf("%lf\n", z / (1 + z));
	return z / (1.0 + z);
}

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 *
 * IMPORTANT: You MAY NOT use any array brackets (i.e. [ and ]) and
 * you MAY NOT declare any arrays or allocate any storage with malloc().
 * The purpose of this restriction is to force you to use pointers.
 * Variables to hold the pathname of the current file or directory
 * as well as other data have been pre-declared for you in const.h.
 * You must use those variables, rather than declaring your own.
 * IF YOU VIOLATE THIS RESTRICTION, YOU WILL GET A ZERO!
 */

/**
 * DTMF generation main function.
 * DTMF events are read (in textual tab-separated format) from the specified
 * input stream and audio data of a specified duration is written to the specified
 * output stream.  The DTMF events must be non-overlapping, in increasing order of
 * start index, and must lie completely within the specified duration.
 * The sample produced at a particular index will either be zero, if the index
 * does not lie between the start and end index of one of the DTMF events, or else
 * it will be a synthesized sample of the DTMF tone corresponding to the event in
 * which the index lies.
 *
 *  @param events_in  Stream from which to read DTMF events.
 *  @param audio_out  Stream to which to write audio header and sample data.
 *  @param length  Number of audio samples to be written.
 *  @return 0 if the header and specified number of samples are written successfully,
 *  EOF otherwise.
 */
AUDIO_HEADER empty_header;
int dtmf_generate(FILE *events_in, FILE *audio_out, uint32_t length) {
	int starting = 0;
	int ending = 0;
	int row_freq = 0;
	int col_freq = 0;
	double pi = M_PI;
	double audio_frame_rate = 8000;

	empty_header.magic_number = AUDIO_MAGIC;
	empty_header.data_offset = 24;
	empty_header.data_size = length * 2;
	empty_header.encoding = 3;
	empty_header.sample_rate = 8000;
	empty_header.channels = 1;

	if (audio_write_header(stdout, &empty_header) == EOF) {
		printf("Line 136");
		return EOF;
	}

	// getting the starting number
	char a = fgetc(events_in);
	if (a != EOF) {
		while(a != '\t') {
			if (a == EOF) { break; }
			if (char_to_int(a) == -1) {
				return EOF;
			}
			starting = starting * 10 + char_to_int(a);
			a = fgetc(events_in);
		}

		a = fgetc(events_in);
		while(a != '\t') {
			if (a == EOF) { 
				printf("Line 155");
				break; 
				}
			if (char_to_int(a) == -1) {
				return EOF;
			}
			ending = ending * 10 + char_to_int(a);
			a = fgetc(events_in);
		}

		a = fgetc(events_in);
		if (a == EOF) {
			printf("line 167");
			return EOF; }
		row_freq = get_row_frequency(a);
		col_freq = get_col_frequency(a);
		if (row_freq == -1 || col_freq == -1) {
			printf("line 172");
			return EOF;
		}
		fgetc(events_in); // Get rid of \n
		if (a == EOF){
			printf("line 177");
			return EOF; }
	}

	// printf("%d %d\n", starting, ending);
	// printf("%d %d\n", row_freq, col_freq);
	FILE* opened_file = 0;
	if (noise_file) {
		opened_file = fopen(noise_file, "r");
		if (!opened_file) {
			printf("Invalid file path\n");
			return EOF;
		}
		if (audio_read_header(opened_file, &empty_header) == EOF) {
			// The file provided was in an incorrect format
			printf("File provided was in incorrect format\n");
			return EOF;
		}
	}

	for (int i = 0; i < length; i++) {
		double dtmf = 0;
		if (i >= starting && i < ending) {
			double a = cos(2.0 * pi * row_freq * i / audio_frame_rate);
			double b = cos(2.0 * pi * col_freq * i / audio_frame_rate);
			double c = a * 0.5 + b * 0.5;
			// printf("%lf %lf %d\n", a, b, INT16_MAX);
			// printf("%d\n", INT16_MAX);s
			dtmf = c * INT16_MAX;
			// printf("%d\n", dtmf);
		}

		if (i > ending) {
			starting = 0;
			ending = 0;

			char a = fgetc(events_in);
			if (a != EOF) {
				while(a != '\t') {
					if (a == EOF) { 
						printf("line 217");
						break; }
					if (char_to_int(a) == -1) {
						printf("line 220");
						return EOF;
					}
					starting = starting * 10 + char_to_int(a);
					a = fgetc(events_in);
				}

				a = fgetc(events_in);
				while(a != '\t') {
					if (a == EOF) { 
						printf("line 230");
						break; }
					if (char_to_int(a) == -1) {
						printf("line 233");
						return EOF;
					}
					ending = ending * 10 + char_to_int(a);
					a = fgetc(events_in);
				}

				a = fgetc(events_in);
				if (a == EOF) {
					printf("line 242");
					return EOF; }
				row_freq = get_row_frequency(a);
				col_freq = get_col_frequency(a);
				if (row_freq == -1 || col_freq == -1) {
					printf("line 247");
					return EOF;
				}
				fgetc(events_in); // Get rid of \n
				if (a == EOF) { 
					printf("line 252");
					return EOF; }
			}
		}

		if (opened_file) {
			double w = get_w();
			int16_t i_real;
			// printf("\nW: %lf\n", w);
			int i = audio_read_sample(opened_file, &i_real);
			if (i != EOF) {
				// printf("\nNoise: %lf\n", i_real * w);
				// printf("Value: %lf\n", dtmf * (1 - w));
				int z = dtmf * (1 - w) + i_real * w;
				// printf("Final: %d\n", z);
				dtmf = z;
				// printf("Final: %d\n", dtmf);
			}
		}

		int dtmf_int = dtmf;
		dtmf_generate_helper(dtmf_int, audio_out);
	}

    return 0;
}

int i_to_freq(int i) {
	if (i == 0) {return 	697; }
	if (i == 1) {return 	770; }
	if (i == 2) {return 	852; }
	if (i == 3) {return 	941; }
	if (i == 4) {return 	1209; }
	if (i == 5) {return 	1336; }
	if (i == 6) {return 	1477; }
	if (i == 7) {return 	1633; }
	return -1;
}

char row_col_to_char(int i, int j) {
	char c = *(j+*(i + dtmf_symbol_names));
	return c;
}

/**
 * DTMF detection main function.
 * This function first reads and validates an audio header from the specified input stream.
 * The value in the data size field of the header is ignored, as is any annotation data that
 * might occur after the header.
 *
 * This function then reads audio sample data from the input stream, partititions the audio samples
 * into successive blocks of block_size samples, and for each block determines whether or not
 * a DTMF tone is present in that block.  When a DTMF tone is detected in a block, the starting index
 * of that block is recorded as the beginning of a "DTMF event".  As long as the same DTMF tone is
 * present in subsequent blocks, the duration of the current DTMF event is extended.  As soon as a
 * block is encountered in which the same DTMF tone is not present, either because no DTMF tone is
 * present in that block or a different tone is present, then the starting index of that block
 * is recorded as the ending index of the current DTMF event.  If the duration of the now-completed
 * DTMF event is greater than or equal to MIN_DTMF_DURATION, then a line of text representing
 * this DTMF event in tab-separated format is emitted to the output stream. If the duration of the
 * DTMF event is less that MIN_DTMF_DURATION, then the event is discarded and nothing is emitted
 * to the output stream.  When the end of audio input is reached, then the total number of samples
 * read is used as the ending index of any current DTMF event and this final event is emitted
 * if its length is at least MIN_DTMF_DURATION.
 *
 *   @param audio_in  Input stream from which to read audio header and sample data.
 *   @param events_out  Output stream to which DTMF events are to be written.
 *   @return 0  If reading of audio and writing of DTMF events is sucessful, EOF otherwise.
 */
int output_event(int start, int end, char c, FILE* output) {
	if (end - start < 250) {
		// not long enough
		return 0;
	}
	if (!c) {
		return 0;
	}
	fprintf(output, "%d\t%d\t%c\n", start, end, c);
	// printf("%d\t%d\t%c\n", start, end, c);
	return 0;
}


int dtmf_detect(FILE *audio_in, FILE *events_out) {
	if (audio_read_header(audio_in, &empty_header) == EOF) {
		return EOF;
	}
	int starting_block = 0;
	int current_block = 0;
	char previous_event = 0;

	while (1) {
	// if (current_block > 0) { break; }
	// printf("----------------\n");
	// intializing the stuff
		// printf("%d\n", current_block);
    for (int i = 0; i < 8; i++) {
    	int frequency = i_to_freq(i);
    	int N = block_size;
    	double k = frequency / 8000.0 * N;
    	// printf("%dHz -> %lf\n", frequency, k);
    	// intializing the state
    	GOERTZEL_STATE* x = (i + goertzel_state);
    	goertzel_init(x, N, k);
    }

    for (int i = 0; i < block_size - 1; i++) {
    	// printf("%d\n", i);
    	int16_t dtmf;
    	int result = audio_read_sample(audio_in, &dtmf);
    	if (result == EOF) {
    		break;
    	}
    	current_block += 1;
    	// printf("%d\n", dtmf);
    	for (int j = 0; j < 8; j++) {
    		GOERTZEL_STATE* x = (j + goertzel_state);
    		// if (j == 0) {
    		// 	printf("%lf\n", x->s0);
    		// }
    		goertzel_step(x, 1.0 * dtmf / INT16_MAX);
    		// if (j == 0) {
    		// 	printf("%lf %lf %lf\n", x->s0, x->s1, x->s2);
    		// }
    	}
    }

    // final step
    int16_t dtmf;
    int result = audio_read_sample(audio_in, &dtmf);
    if (result == EOF) {
    	break;
    }
    // printf("%d\n", dtmf);
    current_block += 1;
    for (int j = 0; j < 8; j++) {
    	GOERTZEL_STATE* x = (j + goertzel_state);
    	double strength = goertzel_strength(x, 1.0 * dtmf / INT16_MAX);
    	// printf("%d\n", current_block);
    	// printf("%lf\n", strength);
    	*(j + goertzel_strengths) = strength;
    }

    int row = 0;
    int col = 4;

    for (int j = 0; j < 8; j++) {
	    // getting the strongest row and column index
	    // printf("%lf\n", *(j + goertzel_strengths));
	    double value = *(j + goertzel_strengths);
    	// printf("%lf\n", value);
    	if (j > 3) {
    		// columns
    		if (value > *(col + goertzel_strengths)) {
    			col = j;
    		}
    	} else {
    		// rows
    		if (value > *(row + goertzel_strengths)) {
    			row = j;
    		}
    	}
    }

    double row_value = *(row + goertzel_strengths);
    double col_value = *(col + goertzel_strengths);

    // printf("row: %lf, col: %lf\n", row_value, col_value);
    // printf("row: %d, col: %d\n", row, col);

    int is_valid = 1;

    if (row_value + col_value >= .01) {
    	// ratio test
    	double ratio = row_value / col_value;
    	double four_db = FOUR_DB;
    	if (ratio >= 1/four_db && ratio <= four_db) {
    		// 6dB test
    		double six_db = SIX_DB;
    		for (int i = 0; i < 4; i++) {
    			if (i == row) {
    				continue;
    			} else {
    				double value = *(i + goertzel_strengths);
    				double new_ratio = row_value / value;
    				if (new_ratio < six_db) {
    					is_valid = 0;
    					break;
    				}
    			}
    		}

    		for (int i = 4; i < 8; i++) {
    			if (i == col) {
    				continue;
    			} else {
    				double value = *(i + goertzel_strengths);
    				double new_ratio = col_value / value;
    				if (new_ratio < six_db) {
    					// printf("col_value: %lf  ratio: %lf\n", col_value, new_ratio);
    					is_valid = 0;
    					break;
    				}
    			}
    		}
    	} else {
    		// printf("strongest row: %lf | strongest col: %lf\n", row_value, col_value);
    		is_valid = 0;
    	}
    } else {
    	// printf("Broke at B\n");
    	is_valid = 0;
   	}

	if (is_valid) {
		// THIS IS TRUE ONLY IF WE HAVE A VALID EVENT
		char event = row_col_to_char(row, col - 4);
		if (event == previous_event || previous_event == 0) {
			previous_event = event;
		} else {
			output_event(starting_block, current_block - block_size, previous_event, events_out);
			starting_block = current_block - block_size;
			previous_event = event;
		}
	} else {
		// printf("Current block: %d\n", current_block);
		output_event(starting_block, current_block - block_size, previous_event, events_out);
		starting_block = current_block;
		previous_event = 0;
	}
	} // ending the while loop
	output_event(starting_block, current_block, previous_event, events_out);
    return EOF;
}

int check_str_equal(char* str1, char* str2) {
	if((*str1==*str2)&&(*(str1+1)==*(str2+1))){
		return 1;
	}
	else{
		return 0;
	}
}

int convert_str_to_int(char* str_num) {
	int num = 0;
	int is_negative = 0;

	if (*str_num == '-') {
		// it is a negative number
		is_negative = 1;
		str_num += 1;
	}

	while (*str_num != '\0') {
		int digit = char_to_int(*str_num);
		num = num * 10 + digit;
		str_num += 1;
	}

	if (is_negative) {
		num = -1 * num;
	}

	return num;
}

int is_valid_str_to_int(char* str_num) {
	if (*str_num == '-') {
		// it is a negative number
		str_num += 1;
	}

	while (*str_num != '\0') {
		int digit = char_to_int(*str_num);
		if (digit == -1) {
			return 0;
		}
		str_num += 1;
	}

	return 1;
}



/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the operation mode of the program (help, generate,
 * or detect) will be recorded in the global variable `global_options`,
 * where it will be accessible elsewhere in the program.
 * Global variables `audio_samples`, `noise file`, `noise_level`, and `block_size`
 * will also be set, either to values derived from specified `-t`, `-n`, `-l` and `-b`
 * options, or else to their default values.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected program operation mode, and global variables `audio_samples`,
 * `noise file`, `noise_level`, and `block_size` to contain values derived from
 * other option settings.
 */
int validargs(int argc, char **argv)
{
	if(argc<2){
		return -1;
	}
	if (check_str_equal(*(argv+1), "-h")) {
		// -h was used
    	global_options = 1;
    	return 0;
	}
	if (check_str_equal(*(argv+1), "-g")) {
		// -g was used
		global_options = 2;
		argv += 1;
		argc -= 1;

		int t_command_used = 0;
		int n_command_used = 0;
		int l_command_used = 0;

		int t_command_value;
		char* n_command_value;
		int l_command_value;

		while (argc > 2) {
			char* command = *(argv+1);
			char* argument = *(argv+2);

			if (argc % 2 == 0) {
				// Means there is not a corresponding argument for each tag
				return -1;
			}
			if (check_str_equal(command, "-t")) {
				if (t_command_used) {
					return -1;
				}
				t_command_used = 1;
				argv += 2;
				argc -= 2;
				if (is_valid_str_to_int(argument)) {
					t_command_value = convert_str_to_int(argument) * 8;
					if (t_command_value < 0) {
						return -1;
					}
				} else {
					return -1;
				}
				continue;
			} else if (check_str_equal(command, "-n")) {
				if (n_command_used) {
					return -1;
				}
				n_command_used = 1;
				argv += 2;
				argc -= 2;
				n_command_value = argument;
				continue;
			} else if (check_str_equal(command, "-l")) {
				if (l_command_used) {
					return -1;
				}
				l_command_used = 1;
				argv += 2;
				argc -= 2;
				if (is_valid_str_to_int(argument)) {
					l_command_value = convert_str_to_int(argument);
					// printf("%d\n", l_command_value);
					if (l_command_value < -30 || l_command_value > 30) {
						return -1;
					}
				} else {
					return -1;
				}
				continue;
			} else {
				return -1;
			}
		}

		if (!t_command_used) {
			audio_samples = 8000;
		} else {
			audio_samples = t_command_value;
		}
		if (!l_command_used) {
			noise_level = 0;
		} else {
			noise_level = l_command_value;
		}
		if (!n_command_used) {
			noise_file = 0;
		} else {
			noise_file = n_command_value;
		}

		// printf("noise_file: %s | audio_samples: %d | noise_level: %d\n", noise_file, audio_samples, noise_level);

    	return 0;
	}
	if (check_str_equal(*(argv+1), "-d")) {
		// -d was used
		global_options = 4;
		argv += 1;
		argc -= 1;

		// if (argc > 2) {//why??
		// 	return -1;
		// }

		if (argc == 1) {
			block_size = 100;
			return 0;
		}
		if(argc!=3){
			return -1; //for it to have the -b option, must be 4 args
		}

		char* command = *(argv+1);
		char* argument = *(argv+2);

		if (check_str_equal(command, "-b")) {
			if (is_valid_str_to_int(argument)) {
				int size = convert_str_to_int(argument);
				if (size < 10 || size > 1000) {
					return -1;
				}
				block_size = size;
			} else {
				return -1;
			}
		} else {
			return -1;
		}

		// printf("%d\n", block_size);

    	return 0;
	}

    return -1;
}
