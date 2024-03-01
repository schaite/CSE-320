#include <stdio.h>

#include "audio.h"
#include "debug.h"

int audio_read_sample(FILE *in, int16_t *samplep) {
	int16_t a = fgetc(in);
	int16_t b = fgetc(in);
	if (a == EOF || b == EOF) {
		return EOF;
	}
	int16_t num = (a << 8) + b;
	*(samplep) = num;
    return 0;
}

int audio_write_sample(FILE *out, int16_t sample) {
    int16_t a = sample >> 8;
    int16_t b = (sample<<8)>>8;

	if (fputc(a, out) == EOF) {
		return EOF;
	}
	if (fputc(b, out) == EOF) {
		return EOF;
	}

    return 0;
}

uint32_t audio_reader_helper(FILE* in) {
	int16_t a;
	int16_t b;

	if (audio_read_sample(in, &a) == EOF || audio_read_sample(in, &b) == EOF) {
		return EOF;
	}

	uint32_t sum_int = a;
	sum_int <<= 16;
	sum_int += (b & 0xffff);

	return sum_int;
}

int audio_read_header(FILE *in, AUDIO_HEADER *hp) {
	uint32_t magic_number = audio_reader_helper(in);
	uint32_t data_offset = audio_reader_helper(in);
	uint32_t data_size = audio_reader_helper(in);
	uint32_t encoding = audio_reader_helper(in);
	uint32_t sample_rate = audio_reader_helper(in);
	uint32_t channels = audio_reader_helper(in);

	if (magic_number == EOF || data_offset == EOF || data_size == EOF || encoding == EOF || sample_rate == EOF || channels == EOF) {
		return EOF;
	}
	hp->magic_number = magic_number;
	hp->data_offset = data_offset;
	hp->data_size = data_size;
	hp->encoding = encoding;
	hp->sample_rate = sample_rate;
	hp->channels = channels;

	if (magic_number != AUDIO_MAGIC || encoding != PCM16_ENCODING || sample_rate != AUDIO_FRAME_RATE || channels != AUDIO_CHANNELS) {
		// printf("%d\n", magic_number);
		return EOF;
	}
	if (data_offset < 24) {
		return -1;
	}
	// printf("%d %d %d %d %d \n", hp->data_offset, hp->data_size, hp->encoding, hp->sample_rate, hp->channels);

	int num_left = hp->data_offset - 24;

	while (num_left > 0) {
		int a = fgetc(in);
		if (a == EOF){
			return EOF;
		}
		num_left -= 1;
	}

    return 0;
}

int audio_write_header_helper(uint32_t num, FILE* out) {
	int16_t a = num >> 16;
	int16_t b = (num << 16)>>16;

	if (audio_write_sample(out, a) == EOF || audio_write_sample(out, b) == EOF) {
		return EOF;
	}

	return 0;
}

int audio_write_header(FILE *out, AUDIO_HEADER *hp) {
	if (audio_write_header_helper(hp->magic_number, out) == EOF) {
		return EOF;
	}
	if (audio_write_header_helper(hp->data_offset, out) == EOF) {
		return EOF;
	}
	if (audio_write_header_helper(hp->data_size, out) == EOF) {
		return EOF;
	}
	if (audio_write_header_helper(hp->encoding, out) == EOF) {
		return EOF;
	}
	if (audio_write_header_helper(hp->sample_rate, out) == EOF) {
		return EOF;
	}
	if (audio_write_header_helper(hp->channels, out) == EOF) {
		return EOF;
	}
    return 0;
}
