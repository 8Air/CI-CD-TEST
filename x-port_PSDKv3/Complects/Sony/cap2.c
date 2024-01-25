#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include <jpeglib.h>
#include <stdint.h>

#include <x264.h>

#include "f420.c"

#include "common.h"

uint32_t width, height;

uint32_t bmp_size;
uint8_t *bmp_buffer;
uint8_t *bmp_buffer_ready;
uint8_t* i420[3];
uint8_t* i420_ready[3];
uint32_t h264_size;
uint8_t* *h264_buffer;
uint8_t* *h264_buffer_ready;

int rgbStride[3]={0}, i420Stride[3];

x264_t *enc;
x264_param_t enc_param={0};
x264_picture_t enc_pic={0};
x264_picture_t enc_pic_out;

#define MAX_JPG_SZ 1024*1024

void dumparr(char *outfile, uint8_t *data, uint32_t sz)
{
   FILE *output = NULL;
    output = fopen(outfile, "wb");
    fwrite(data, sz, 1, output);
    fclose(output);
}

void dumparradd(char *outfile, uint8_t *data, uint32_t sz)
{
   FILE *output = NULL;
    output = fopen(outfile, "wb");
    fseek(output, 0, SEEK_END);
    fwrite(data, sz, 1, output);
    fclose(output);
}



void do_convert_my()
{
  i420Stride[0] = width;
  i420Stride[1] = width / 2;
  i420Stride[2] = width / 2;
  i420[0] = (uint8_t*)malloc((size_t)i420Stride[0] * height);
  i420[1] = (uint8_t*)malloc((size_t)i420Stride[1] * height);
  i420[2] = (uint8_t*)malloc((size_t)i420Stride[2] * height);
  Bitmap2Yuv420p_calc2(i420[0],i420[1],i420[2], bmp_buffer_ready, width, height);
}


void init_compress()
{


	x264_param_default_preset(&enc_param, "veryfast", "zerolatency");


	enc_param.i_csp = X264_CSP_I420;//X264_CSP_RGB;
	enc_param.i_width  = width;
	enc_param.i_height = height;

	enc_param.i_threads = 4;
	enc_param.i_fps_num = 10;
	enc_param.i_fps_den = 1;
	enc_param.i_keyint_max = 10;
	enc_param.i_keyint_min = 0;
	enc_param.i_bframe = 0;
        enc_param.i_bframe_pyramid = 0;
	enc_param.b_intra_refresh = 0;
	enc_param.i_frame_reference = 1;

	enc_param.b_open_gop = 0;
	enc_param.b_vfr_input = 0;
	enc_param.b_repeat_headers = 1;
	enc_param.b_annexb = 1;
	enc_param.rc.i_rc_method = X264_RC_ABR;
	enc_param.rc.i_bitrate = 512;
//	enc_param.i_nal_hrd = X264_NAL_HRD_CBR;
	enc_param.rc.i_vbv_max_bitrate = 1024;
	enc_param.rc.i_vbv_buffer_size = 1024;

//      enc_param.rc.f_rf_constant = 28;
//	enc_param.rc.f_rf_constant_max = 40;
	enc_param.i_log_level = X264_LOG_NONE;
//	enc_param.i_log_level = X264_LOG_DEBUG;

	if( x264_param_apply_profile( &enc_param, "baseline" ) < 0 )
	{
		printf("ERROR x264_param_apply_profile\n");
		exit(EXIT_FAILURE);
	}
	
	if( x264_picture_alloc(&enc_pic, enc_param.i_csp, enc_param.i_width, enc_param.i_height ) < 0 )
	{
		printf("ERROR x264_picture_alloc\n");
		exit(EXIT_FAILURE);
	}
	enc = x264_encoder_open( &enc_param );
	if( !enc )
	{
		printf("ERROR x264_encoder_open\n");
		exit(EXIT_FAILURE);
	}

}

void do_compress()
{
	static x264_nal_t *nal;
	static int i_nal, i_frame_size;
	static unsigned int i_frame=0;

        enc_pic.i_pts = i_frame++;
	enc_pic.img.i_plane=3;
	enc_pic.img.plane[0]=i420_ready[0];
	enc_pic.img.plane[1]=i420_ready[1];
	enc_pic.img.plane[2]=i420_ready[2];
	enc_pic.img.i_stride[0]=i420Stride[0];
	enc_pic.img.i_stride[1]=i420Stride[1];
	enc_pic.img.i_stride[2]=i420Stride[2];
        i_frame_size = x264_encoder_encode(enc, &nal, &i_nal, &enc_pic, &enc_pic_out );
        if( i_frame_size < 0 )
		printf("ERROR x264_encoder_encode, %i\n",i_frame_size);
        else 
	{
		h264_buffer=nal[0].p_payload;
		h264_size=i_frame_size;
//		printf("OK, frame #%i, %ib\n",i_frame,i_frame_size);
        }
//	dumparradd("test.h264",encframed,encframesz);

}


void free_compressor()
{

    x264_encoder_close( enc );
    x264_picture_clean( &enc_pic );
}


void do_jpeg() 
{

	int32_t rc, i, j;
	uint32_t row_stride, pixel_size;
	struct stat file_info;
	unsigned long jpg_size;
	unsigned char *jpg_buffer;
	FILE *jpg_file;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
        struct stat fstat;

	jpg_buffer = (unsigned char*) malloc(MAX_JPG_SZ);


        if (stat(photoFile ,&fstat)!=0) usleep(15000);
        if (stat(photoFile ,&fstat)!=0) usleep(30000);
        if (stat(photoFile ,&fstat)!=0) usleep(60000);
        if (stat(photoFile ,&fstat)!=0) usleep(100000);
        if (stat(photoFile ,&fstat)!=0) usleep(100000);
        if (stat(photoFile ,&fstat)!=0) usleep(100000);
        if (stat(photoFile ,&fstat)!=0) usleep(100000);
        if (stat(photoFile ,&fstat)!=0) usleep(100000);
        if (stat(photoFile ,&fstat)!=0) usleep(100000);

        if (stat(photoFile,&fstat)==0) {
                jpg_file=fopen(photoFile, "rb");
		rc = fread(jpg_buffer, 1, MAX_JPG_SZ, jpg_file);
		jpg_size=rc;
		fclose(jpg_file);
                remove(photoFile);
	} else {

                jpg_file=fopen(apptestCameraPhotoFile, "rb");
		rc = fread(jpg_buffer, 1, MAX_JPG_SZ, jpg_file);
		jpg_size=rc;
		fclose(jpg_file);
//		printf("READ BK JPG\n");

	}
	
	cinfo.err = jpeg_std_error(&jerr);	
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);

	rc = jpeg_read_header(&cinfo, TRUE);
	if (rc != 1) {
		printf("File does not seem to be a normal JPEG\n");
		exit(EXIT_FAILURE);
	}

	jpeg_start_decompress(&cinfo);
	
	width = cinfo.output_width;
	height = cinfo.output_height;
	pixel_size = cinfo.output_components;

	bmp_size = width * height * pixel_size;
	bmp_buffer = (unsigned char*) malloc(bmp_size);
	row_stride = width * pixel_size;

	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned char *buffer_array[1];
		buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);

	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	free(jpg_buffer);
	return EXIT_SUCCESS;
}
