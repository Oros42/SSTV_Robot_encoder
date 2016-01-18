/* Simple and fast SSTV encoder for Robot8BW and Robot24BW.
 *
 * Author: Oros
 * 2016-01-18
 * Licence CC0 1.0 Universal
 * 
 * Beacause pySSTV* is realy to slow on a Raspberry-Pi B, I rewrite it in C :-D
 * * pySSTV : https://github.com/dnet/pySSTV
 * 
 * Benchmark on a Raspberry-Pi B :
 * # with pySSTV
 * $ time python -m pysstv --mode Robot8BW photo.jpg output.wav
 * 
 * real	0m19.954s
 * user	0m19.360s
 * sys	0m0.170s
 * 
 * # with this program :
 * $ time ./SSTV_Robot_encoder photo.jpg output.wav Robot8BW
 * 
 * real	0m1.039s
 * user	0m0.980s
 * sys	0m0.050s
 * 
 * How to buid :
 * apt-get install gcc libgd-dev
 * gcc SSTV_Robot_encoder.c -o SSTV_Robot_encoder -lgd -lm
 * 
 * Who to run :
 * ./SSTV_Robot_encoder INPUT_IMAGE [OUTPUT_WAVE [MODE]]
 */

#include <gd.h>
#include <stdio.h>
#include <math.h>

typedef struct {
	int NAME;
	unsigned char VIS_CODE;
	int WIDTH;
	int HEIGHT;
	int SYNC;
	int SCAN;
} struct_mode;

const struct_mode Robot8BW = { 8, 0x02, 160, 120, 7, 60 };
const struct_mode Robot24BW = { 24, 0x0A, 320, 240, 7, 93 };
struct_mode MODE;


const int SAMPLES_PER_SEC = 48000;
const float SAMPLES_PER_MSEC = 48; // SAMPLES_PER_SEC / 1000
const double FACTOR = 0.0001308996938995747; // 2 * 3.141592653589793 / SAMPLES_PER_SEC
const int LOWEST = -32768; // = - 2 ** 16 // 2
const int HIGHEST = 32767; // = 2 ** 16 // 2 -17

const int FREQ_SYNC = 1200;
const int FREQ_VIS_START = 1900;
const int FREQ_VIS_BIT0 = 1300;
const int FREQ_VIS_BIT1 = 1100;
const int FREQ_BLACK = 1500;
const int FREQ_WHITE = 2300;
const int FREQ_RANGE = 800; // FREQ_WHITE - FREQ_BLACK
const int FREQ_FSKID_BIT1 = 1900;
const int FREQ_FSKID_BIT0 = 2100;

const float MSEC_VIS_SYNC = 10;
const float MSEC_VIS_START = 300;
const float MSEC_VIS_BIT = 30;
const float MSEC_FSKID_BIT = 22;

double offset = 0;
unsigned char *wave_data_ptr;

void print_usage(char *argv[]) {
	printf("Usage : %s INPUT_IMAGE [OUTPUT_WAVE [MODE]]\n\n"
		"INPUT_IMAGE : should be a jpeg or png or gif or bmp in back and white\n"
		"OUTPUT_WAVE : by default sstv_robot_8BW.wav\n"
		"MODE : Robot8BW ou Robot24BW. By default Robot8BW\n\n"
		"INPUT_IMAGE size :\n"
		"\tRobot8BW : 160x120\n"
		"\tRobot24BW : 320x240\n\n"
		"Examples :\n"
		"%s photo_160x120.jpg\n"
		"%s photo_160x120.jpg sstv_robot_8BW.wav\n"
		"%s photo_160x120.jpg sstv_robot_8BW.wav Robot8BW\n"
		"%s photo_320x240.png sstv_robot_24BW.wav Robot24BW\n"
		, argv[0], argv[0], argv[0], argv[0], argv[0]);
}

gdImagePtr getGdImage(const char * img_name) {
	gdImagePtr gd_img;
	FILE *f;
	f = fopen(img_name, "rb");
	if (f == NULL) {
		fprintf(stderr, "\033[31mCan't open %s\033[0m\n", img_name);
		return NULL;
	}
	int c;
	c = fgetc(f);
	fseek(f, 0, SEEK_SET);
	if(c==0xFF) {
		// jpeg FF D8 FF E0
		gd_img = gdImageCreateFromJpeg(f);
	}else if(c==0x89) {
		// png 89 50 4E 47
		gd_img = gdImageCreateFromPng(f);
	}else if(c==0x47) {
		// gif 47 49 46 38
		gd_img = gdImageCreateFromGif(f);
	}else if(c==0x42) {
		// bmp 42 4D 0A 02
		gd_img = gdImageCreateFromWBMP(f);
	}else{
		fprintf(stderr, "\033[31mUnknow file type for %s\033[0m\n", img_name);
		return NULL;
	}
	fclose(f);
	return gd_img;
}

void gen_samples(int freq, float msec) {
	double freq_factor = FACTOR * freq;
	int i, n, sample;
	for(i=0; i < SAMPLES_PER_MSEC * msec; i++) {
		// 32768 = 2 ** 16 // 2
		sample = (int) ( 32768 * sin( i * freq_factor + offset) );
		if(sample <= LOWEST) {
			// LOWEST
			*wave_data_ptr = 0x00;
			wave_data_ptr++;
			*wave_data_ptr = 0x80;
		}else if(sample >= HIGHEST) {
			// HIGHEST
			*wave_data_ptr = 0xff;
			wave_data_ptr++;
			*wave_data_ptr = 0x7f;
		}else{
			for(n=0; n < 8; n++) {
				*wave_data_ptr |= (sample & 1) << n;
				sample>>=1;
			}
			wave_data_ptr++;
			for(n=0; n<8; n++){
				*wave_data_ptr |= (sample & 1) << n;
				sample>>=1;
			}
		}
		wave_data_ptr++;
	}
	offset+=i*freq_factor;
}

int make_sstv_wave(gdImagePtr gd_img, FILE *wav) {
	unsigned char wave_header[44] = {
		/* chunk descriptor */
		0x52, 0x49, 0x46, 0x46, /* RIFF */
		0x64, 0x1C, 0x0D, 0x00, /* chunk size : 859236 (Robot8BW) */
		0x57, 0x41, 0x56, 0x45, /* WAVE */

		/* fmt subchunk */
		0x66, 0x6D, 0x74, 0x20, /* fmt  */
		0x10, 0x00, 0x00, 0x00, /* subchunk 1 size : 16 */
		0x01, 0x00, 			/* audio format : 1 */
		0x01, 0x00, 			/* num channels : 1*/
		0x80, 0xBB, 0x00, 0x00, /* sample rate : 48000 */
		0x00, 0x77, 0x01, 0x00, /* byte rate : 96000 */
		0x02, 0x00, 			/* block align : 2 */
		0x10, 0x00, 			/* bits per sample : 16*/

		/* data subchunk */
		0x64, 0x61, 0x74, 0x61, /* data */
		0x40, 0x1C, 0x0D, 0x00  /* subchunk 2 size : 859200 (Robot8BW) */
	};
	int subchunk_size;
	if(MODE.NAME == 24) {
		/* Robot24BW */
		/* chunk size : 2391396 */ 
		wave_header[5] = 0x7D;
		wave_header[6] = 0x24;

		/* subchunk 2 size : 2391360 */
		wave_header[41] = 0x7D;
		wave_header[42] = 0x24;
		subchunk_size = 2391360;
	}else{
		/* Robot8BW */
		subchunk_size = 859200;
	}

	unsigned char wave_data[subchunk_size];
	wave_data_ptr = wave_data;

	if (fwrite(&wave_header, 1, 44, wav) != 44) {
		fprintf(stderr, "\033[31mCan't write header wave file\033[0m\n");
		return -1;
	}

	/* SSTV header */
	gen_samples(FREQ_VIS_START, MSEC_VIS_START);
	gen_samples(FREQ_SYNC, MSEC_VIS_SYNC);
	gen_samples(FREQ_VIS_START, MSEC_VIS_START);
	gen_samples(FREQ_SYNC, MSEC_VIS_BIT);// start bit

	int i, bit, num_ones;
	bit = num_ones = 0;
	for(i=0; i<7; i++){
		bit = (MODE.VIS_CODE >> i) & 1;
		num_ones += bit;
		if(bit == 1){
			gen_samples(FREQ_VIS_BIT1, MSEC_VIS_BIT);
		}else{
			gen_samples(FREQ_VIS_BIT0, MSEC_VIS_BIT);
		}
	}
	if(num_ones % 2 == 1){
		gen_samples(FREQ_VIS_BIT1, MSEC_VIS_BIT);
	}else{
		gen_samples(FREQ_VIS_BIT0, MSEC_VIS_BIT);
	}
	gen_samples(FREQ_SYNC, MSEC_VIS_BIT); // stop bit

	/* image data */
	float msec_pixel = (float) MODE.SCAN / (float) MODE.WIDTH;
	int line, col;
	for(line=0; line < MODE.HEIGHT; line++){
		gen_samples(FREQ_SYNC, MODE.SYNC);
		for(col=0; col < MODE.WIDTH; col++){
			gen_samples(FREQ_BLACK + FREQ_RANGE * gdImageGreen(gd_img, gdImageGetPixel(gd_img, col, line)) / 255, msec_pixel);
		}
	}

	const size_t n = sizeof wave_data / sizeof wave_data[0];
	if (fwrite(&wave_data, sizeof wave_header[0], n, wav) != n) {
		fprintf(stderr, "\033[31mCan't write data wave file\033[0m\n");
		return -1;
	}
	return 1;
}


int main(int argc, char *argv[]) {
	if(argc < 2 ) {
		print_usage(argv);
		return -1;
	}

	/* set name of output wave */
	const char * output_wave;
	if(argc == 3) {
		output_wave = argv[2];
	}else{
		output_wave = "sstv_robot_8BW.wav";
	}

	/* open image */
	gdImagePtr gd_img;
	const char * img_name;
	img_name = argv[1];
	gd_img = getGdImage(img_name);
	if(gd_img == NULL) {
		fprintf(stderr, "\033[31m%s is not a jpeg or png or gif or bmp file.\033[0m\n", img_name);
		return -1;
	}

	/* set SSTV MODE */
	if(argc == 4) {
		if(strcmp(argv[3],"Robot24BW") == 0){
			MODE = Robot24BW;
		}else{
			MODE = Robot8BW;
		}
	}else{
		MODE = Robot8BW;
	}

	/* check image size */
	if(gdImageSX(gd_img) != MODE.WIDTH || gdImageSY(gd_img) != MODE.HEIGHT) {
		fprintf(stderr, "\033[31mWrong size for %s\033[0m\n", img_name);
		if(MODE.NAME == 8) {
			fprintf(stderr, "For Robot8BW : 160x120\n");
		}else{
			fprintf(stderr, "For Robot24BW : 320x240\n");
		}
		gdImageDestroy(gd_img);
		return -1;
	}

	/* open wave file */
	FILE *wave;
	wave = fopen(output_wave, "wb");
	if (wave == NULL) {
		fprintf(stderr, "\033[31mCan't open %s\033[0m\n", output_wave);
		gdImageDestroy(gd_img);
		return -1;
	}

	/* make SSTV wave */
	make_sstv_wave(gd_img, wave);

	fclose(wave);
	gdImageDestroy(gd_img);
	return 0;
}