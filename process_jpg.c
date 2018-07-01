#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "jpeglib.h"
#include "process_jpg.h"

struct jpeg_decompress_struct cinfo;
struct jpeg_error_mgr jerr;

int read_JPEG_file (char *filename, unsigned char *buffer);
void copy_to_buffer(unsigned char *buffer, int row, unsigned char *data);
void put_scanline_someplace(unsigned char *buffer, int row, FILE *f);

/*This function reads the size of the image from the header and copies
  the value in an image_size struct which is passed as a parameter
 */
int get_image_size(char *filename, image_size *size)
{
    FILE *image;
    const char *fname = filename;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    image = fopen(fname, "rb");
    if(image == NULL) {
        fprintf(stderr, "get image size: can't open the file \n");
        return 0;
    }

    cinfo.err = jpeg_std_error(&jerr);
    //qDebug() << "img second in getsize" << filename << "  " << fname<< endl;

    jpeg_create_decompress(&cinfo); /*initialize decompression object*/
    jpeg_stdio_src(&cinfo, image); /*specify data source (eg, a file)*/
    (void) jpeg_read_header(&cinfo, TRUE); /*read JPEG header*/
    (void) jpeg_start_decompress(&cinfo); /* start decompressor */

    /*Return the size of the image from the header info*/
    size->width = cinfo.output_width;
    size->height = cinfo.output_height;

    /*when done clean up and leave*/
    jpeg_destroy_decompress(&cinfo);
    fclose(image);

    //qDebug() << "leaving get size" << filename<< endl;
    return 1;
}

/* This function decompresses a jpg file (pointed to by the filename parameter)
   and copies the extracted RGB into the input buffer passed as a parameter.
*/

int extract_rgb_from_jpg(char *filename, unsigned char *buffer)
{
    int ret_val;
    //int i;

    assert(buffer != NULL);
    unsigned char *temp_ptr = buffer;

    //qDebug() << "img in extract" << filename << endl;
    if((ret_val = read_JPEG_file(filename, temp_ptr) != 1)) {
        fprintf(stderr, "RGB not extracted \n");
        return 0;
    }

    return 1;
}

int read_JPEG_file(char *filename, unsigned char *data)
{
    /* This struct contains the JPEG decompression parameters and pointers to
    * working space (which is allocated as needed by the JPEG library).
    */

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    /* More variables */
    FILE *infile;         	/* source file */
    FILE *destfile;
    JSAMPARRAY img_buffer;        /* Output row buffer */
    int row_stride;       	/* physical row width in output buffer */
    int i = 0;


    unsigned char *temp_ptr = data;
    char *fname = filename;

    if ((infile = fopen(fname, "rb")) == NULL) {
        fprintf(stderr, "read_JPEG_FILE: can't open %s\n", filename);
        return 0;
    }

    if ((destfile = fopen("decompressed_2", "w")) == NULL) {
        fprintf(stderr, "read_JPEG_FILE: can't open %s\n", filename);
        return 0;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    (void) jpeg_read_header(&cinfo, TRUE);
    (void) jpeg_start_decompress(&cinfo); /* start decompressor */

    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    img_buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
    * loop counter, so that we don't have to keep track ourselves.
    */
    while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */

        (void) jpeg_read_scanlines(&cinfo, img_buffer, 1);
    /* Assume put_scanline_someplace wants a pointer and sample count. */
        copy_to_buffer(img_buffer[0], row_stride, temp_ptr+row_stride*i);
        i++;
        put_scanline_someplace(img_buffer[0], row_stride, destfile);
    }

    (void) jpeg_finish_decompress(&cinfo);/* Step 7: Finish decompression */
    jpeg_destroy_decompress(&cinfo); /*Release JPEG decompression object */
    fclose(infile);
    return 1;
}

void copy_to_buffer(unsigned char *buffer, int row, unsigned char *data)
{
    int i;
    //printf("ptr is %p \n", data);
    for(i = 0; i<row; i++){
        *data++ = buffer[i];
        //printf("%i \n", buffer[i]);
    }
}

void put_scanline_someplace(unsigned char *buffer, int row, FILE *f)
{
    int i;
    for(i = 0; i<row; i++)
        fputc(buffer[i], f);
}
