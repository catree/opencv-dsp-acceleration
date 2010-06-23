/*
 *  ======== opencvtest.c ========
 *  Test application for OPENCVTEST algorithm.
 */

#include <xdc/std.h>
#include <ti/sdo/ce/Engine.h>
#include <ti/sdo/ce/osal/Memory.h>
#include <ti/sdo/ce/trace/gt.h>
#include <string.h>

#include <ti/sdo/ce/CERuntime.h>
#include <ti/sdo/ce/trace/gt.h>

#include <ti/sdo/ce/universal/universal.h>
#include <beagle/algos/opencv/iopencv.h>
#include <beagle/algos/opencv/opencv_beagle.h>

/* Standard Linux headers */
#include <stdio.h>
#include <stdlib.h>

#include "beagle_opencv_API.h"




#ifndef BUFALIGN
#define BUFALIGN 128
#endif

    
static IOPENCV_Params *opencvParams;


/* trace info: module name, mask */
GT_Mask curMask = {0,0};


static String progName     = "app";
static String opencvName  = "opencv";


static void OPENCV_runTests( OPENCV_Operation );
static void OPENCVTEST_runTests(XDAS_Int8 *, XDAS_Int8 *, XDAS_Int8 *, OPENCV_Operation);


/* Universal  Codec Handle */
static UNIVERSAL_Handle codec = NULL;

/* Global flag used to signal when it is time to exit test loop */
int OPENCVTEST_gfxExitThread = 0;

/* Global Quit Function to call */
static Fxn OPENCVTEST_getQuitFxn = NULL;


static int myGetQuitFunction(void)
{
	
	return (0);
}


/*
 *  ======== OPENCV_runTests ========
 *
 *  Execute the test suite and print out benchmark numbers
 *
 */

static void OPENCV_runTests( OPENCV_Operation operation )
{
    
    /*
     * Format for -> Void OPENCVTEST_runTests(XDAS_Int16 *inbuf_1, XDAS_Int16 *inbuf_2, XDAS_Int16 *outbuf, OPENCV_Operation opencvOperation);
     */
    switch ( operation ) {

	   case OPENCV_OPERATION_GEN_TWIDDLE :
    	   	OPENCVTEST_runTests(NULL, NULL, opencvParams->d_ptr, OPENCV_OPERATION_GEN_TWIDDLE);
		break;
           
           case OPENCV_OPERATION_DFT :
 		OPENCVTEST_runTests(NULL, NULL, opencvParams->d_ptr, OPENCV_OPERATION_GEN_TWIDDLE);
		printf("Twiddle-factors Generated....Starting DFT");
		OPENCVTEST_runTests(opencvParams->s_ptr, opencvParams->d_ptr, opencvParams->d_ptr, OPENCV_OPERATION_DFT); 
		break;

	   case OPENCV_OPERATION_SOBEL3x3 :
	   case OPENCV_OPERATION_SOBEL5x5 :
 	   case OPENCV_OPERATION_SOBEL7x7 :
		OPENCVTEST_runTests(opencvParams->s_ptr, NULL, opencvParams->d_ptr, operation); 
		break;
	   default :
		return;
    }


}
     


/*
 *  ======== OPENCVTEST_runTests ========
 *
 *  Execute the test suite and print out benchmark numbers
 *
 */
static void OPENCVTEST_runTests(XDAS_Int8 *inbuf_1, XDAS_Int8 *inbuf_2, XDAS_Int8 *outbuf, OPENCV_Operation opencvOperation)
{
    int	status;
    
    
    XDAS_Int8 *inputBuf1, *inputBuf2;
    XDAS_Int8 *outputBuf;
    XDAS_Int8 *isNull;

    UNIVERSAL_InArgs universalInArgs;
    IOPENCV_InArgs inArgs;
    inArgs.universalInArgs = universalInArgs;
    inArgs.operation = opencvOperation;	
    inArgs.dftSize = opencvParams->s_cols; // dftsize is the N size not the size of buffer
    inArgs.universalInArgs.size = sizeof(IOPENCV_InArgs); 

    UNIVERSAL_OutArgs outArgs;
    outArgs.size = sizeof(outArgs);

    XDM1_BufDesc universalInBufDesc;
    XDM1_BufDesc universalOutBufDesc;


    inputBuf1 = Memory_contigAlloc(opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels , BUFALIGN);
    inputBuf2 = Memory_contigAlloc(opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels , BUFALIGN);
    outputBuf = Memory_contigAlloc(opencvParams->d_cols * opencvParams->d_rows * opencvParams->d_channels , BUFALIGN);
    if ((inputBuf1 == NULL) || (inputBuf2 == NULL) || (outputBuf == NULL)) {
        printf("App-> ERROR: Memory_contigAlloc for buffers failed");
    }

    isNull = Memory_contigAlloc(sizeof(XDAS_Int8) * 2, BUFALIGN); // currently support only 2 buffer. That is why it is * 2.
    isNull[0] = 1;
    isNull[1] = 1;

    if (inbuf_1) {

       memcpy( inputBuf1, inbuf_1, opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels);
       isNull[0] = 0;
    }
    if (inbuf_2) {

       memcpy( inputBuf2, inbuf_2, opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels);
       isNull[0] = 0;
    }

    /* Passing buffer size and buffer pointer which will be address translated */
    universalInBufDesc.numBufs = 3; 
    universalOutBufDesc.numBufs = 1;
    
    universalInBufDesc.descs[0].bufSize = opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels; 
    universalInBufDesc.descs[0].buf = (XDAS_Int8 *)inputBuf1;


    universalInBufDesc.descs[1].bufSize = opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels; 
    universalInBufDesc.descs[1].buf = (XDAS_Int8 *)inputBuf2;

    universalInBufDesc.descs[2].bufSize = (sizeof(XDAS_Int8) * 2); 
    universalInBufDesc.descs[2].buf = isNull;


    universalOutBufDesc.descs[0].bufSize = opencvParams->d_cols * opencvParams->d_rows * opencvParams->d_channels;
    universalOutBufDesc.descs[0].buf = (XDAS_Int8 *)outputBuf;

   


    status = UNIVERSAL_process(codec, &universalInBufDesc, &universalOutBufDesc, NULL, (IUNIVERSAL_InArgs *) &inArgs, &outArgs);

    if (status != UNIVERSAL_EOK) {
        printf("Error calling OPENCV module:%x\n",status);
        goto exit;
    }

    memcpy( opencvParams->d_ptr, outputBuf, (opencvParams->d_cols * opencvParams->d_rows * opencvParams->d_channels));


    exit:
    
    Memory_contigFree(inputBuf1,  opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels);
    Memory_contigFree(inputBuf2,  opencvParams->s_cols * opencvParams->s_rows * opencvParams->s_channels);
    Memory_contigFree(outputBuf, opencvParams->d_cols * opencvParams->d_rows * opencvParams->d_channels);
    Memory_contigFree(isNull, sizeof(XDAS_Int8) * 2);
}


/*
 *  ======== OPENCVTEST_main ========
 */
void OPENCVTEST_main( int s_type, int s_step, char *s_data, int s_rows, int s_cols, int s_channels, int d_type, int d_step, char *d_data, int d_rows, int d_cols, int d_channels, OPENCV_Operation operation )
{
    char engine_Name[] = "opencvEngine";
    char *engineName = engine_Name;

    if (s_type != CV_8UC1 && s_type != CV_8SC1) {
        printf ("Error: Currently only image type of CV_8UC1 and CV_8SC1 are supported.\n");
        return;
    }
    opencvParams = (IOPENCV_Params *)malloc(sizeof(IOPENCV_Params));

    /* Assign OpenCV parameters */
    opencvParams->size 		= sizeof (IOPENCV_Params);
    
    opencvParams->s_type	= s_type;
    opencvParams->s_step	= s_step;
    opencvParams->s_ptr		= (XDAS_Int8 *)s_data;
    opencvParams->s_rows	= s_rows;
    opencvParams->s_cols	= s_cols;
    opencvParams->s_channels 	= s_channels;

    opencvParams->d_type	= d_type;
    opencvParams->d_step	= d_step;
    opencvParams->d_ptr		= (XDAS_Int8 *)d_data;
    opencvParams->d_rows	= d_rows;
    opencvParams->d_cols	= d_cols;
    opencvParams->d_channels   	= d_channels;
    

    /* Inatilaize codec-engine */    

    CERuntime_init();

    GT_create(&curMask, "ZZ");

    /* Enable all trace for this "ZZ" module */
    GT_set("ZZ=01234567");

    GT_0trace(curMask, GT_2CLASS, "main> Welcome to app's main().\n");
   

    Engine_Handle ce = NULL;

    printf("App-> Application started.\n");

    /* Init the global quit function */
    OPENCVTEST_getQuitFxn = myGetQuitFunction;
    
    /* reset, load, and start DSP Engine */
    if ((ce = Engine_open(engineName, NULL, NULL)) == NULL) {
        printf("%s: error: can't open engine %s\n", progName, engineName);
        goto endFailOpenEngine;
    }

    /* allocate and initialize opencv algo on the engine */
    codec = UNIVERSAL_create(ce, opencvName, (IUNIVERSAL_Params *)opencvParams);
    if (codec == NULL) {
        printf( "App-> ERROR: can't open codec %s\n", opencvName);
        goto endFailOpenCodec;
    }
    printf("App-> Codec created.\n");

    /* Enter test app */
    OPENCV_runTests( operation ); 

    printf( "App-> Completed Successfully\n");

    /* teardown the codec */
    if (codec) {
        UNIVERSAL_delete(codec);
    }

endFailOpenCodec:
    /* close the engine */
    if (ce) {
        Engine_close(ce);
    }

endFailOpenEngine:
    printf("app ended.\n");
    
}

