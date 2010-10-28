#include "cv.h"
#include "highgui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>

#ifdef _EiC
#define WIN32
#endif

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;
static CvHaarClassifierCascade* fcascade = 0;

void detect_and_draw( IplImage* image );

bool file = false;
bool show = false;
int min_neighbors=4;
float scale_factor;

const char* cascade_name ="smiled_05.xml";

int main( int argc, char** argv )
{
    CvCapture* capture = 0;
    IplImage *frame, *frame_copy = 0;
    int optlen = strlen("--cascade=");
    const char* input_name;
    
    int i;
    for( i = 1; i < argc; i++ )
    {
        if( !strncmp( argv[i], "--cascade=", optlen ) )
        {
            cascade_name = argv[++i] + optlen;
        }
        else if( !strcmp( argv[i], "-c" ) )
        {
            cascade_name = argv[++i];
        }
        else if( !strcmp( argv[i], "-sf" ) )
        {
            scale_factor = (float) atof( argv[++i] );
        }
        else if( !strcmp( argv[i], "-mn" ) )
        {
            min_neighbors = atoi( argv[++i] );
        }
        else if( !strcmp( argv[i], "-show" ) )
        {
          show =true;
        }
        else 
        {
            input_name = argv[i];
        }
    }

    fcascade = (CvHaarClassifierCascade*)cvLoad("/usr/local/share/opencv/haarcascades/haarcascade_frontalface_alt.xml", 0, 0, 0 );
    cascade = (CvHaarClassifierCascade*)cvLoad( cascade_name, 0, 0, 0 );
    
    if( !cascade )
    {
        fprintf( stderr, "ERROR: Could not load smile classifier cascade\n" );
        fprintf( stderr, "Usage: smileD --cascade=\"<smile_cascade_path>\" [filename|filelist|camera_index]\n" );
        return -1;
    } if (!fcascade) {
        fprintf( stderr, "ERROR: Could not load face detector cascade\n" );
        return -1;
    }
    
    storage = cvCreateMemStorage(0);
    
    if( !input_name || (isdigit(input_name[0]) && input_name[1] == '\0') )
        capture = cvCaptureFromCAM( !input_name ? 0 : input_name[0] - '0' );
    else {
        file=true;
        capture = cvCaptureFromAVI( input_name ); 
    }
    cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_HEIGHT,240);
        cvSetCaptureProperty(capture,CV_CAP_PROP_FRAME_WIDTH,320);
        
    cvNamedWindow( "result", 1 );

    if( capture )
    {
        for(;;)
        {
            if( !cvGrabFrame( capture ))
                break;
            frame = cvRetrieveFrame( capture );
            if( !frame )
                break;
            if( !frame_copy )
                frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
                                            IPL_DEPTH_8U, frame->nChannels );
            if( frame->origin == IPL_ORIGIN_TL )
                cvCopy( frame, frame_copy, 0 );
            else
                cvFlip( frame, frame_copy, 0 );
            
            detect_and_draw( frame_copy );

            if( cvWaitKey( 10 ) >= 0 )
                break;
        }

        cvReleaseImage( &frame_copy );
        cvReleaseCapture( &capture );
    }
    else
    {
        const char* filename = input_name ? input_name : (char*)"lena.jpg";
        IplImage* image = cvLoadImage( filename, 1 );
        if( image )
        {
            detect_and_draw( image );
            if (show) cvWaitKey(0);
            cvReleaseImage( &image );
        }
        else
        {
            /* the last command line argument can be a name of the file containing the list of the image filenames to be processed - one per line */
            FILE* f = fopen( filename, "rt" );
            if( f )
            {
                char buf[1000+1];
                while( fgets( buf, 1000, f ) )
                {
                    int len = (int)strlen(buf);
                    while( len > 0 && isspace(buf[len-1]) )
                        len--;
                    buf[len] = '\0';
                    fprintf(stdout,"\n%s ",buf);
                    image = cvLoadImage( buf, 1 );
                    if( image )
                    {
                        detect_and_draw( image );
                        if (show) cvWaitKey(0);
                        cvReleaseImage( &image );
                    }
                }
                fclose(f);
            }
        }

    }
    
    cvDestroyWindow("result");

    return 0;
}

void detect_and_draw( IplImage* img )
{
    static CvScalar colors[] = 
    {
        {{0,0,255}},
        {{0,128,255}},
        {{0,255,255}},
        {{0,255,0}},
        {{255,128,0}},
        {{255,255,0}},
        {{255,0,0}},
        {{255,0,255}}
    };

    double scale = 1.3;
    IplImage* gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );

    int i;
    cvCvtColor( img, gray, CV_BGR2GRAY );
    cvClearMemStorage( storage );

    CvSeq* faces1 = cvHaarDetectObjects( img, fcascade, storage, 1.1, 2, 0, cvSize(30,30) );
    //printf("%d faces detected\n",faces1->total);
      
     if (faces1->total > 0) {
       CvRect* r = (CvRect*)cvGetSeqElem(faces1, 0);
       CvRect* roi = (CvRect*)cvGetSeqElem(faces1, 0);
       cvRectangle(img,cvPoint(r->x,r->y),cvPoint(r->x+r->width,r->y+r->height),cvScalar(255,0,255,0),1,0,0);

       //we search smile only in middle inferior part of already detected face
       cvSetImageROI(img,cvRect(r->x+(r->width*1/7),r->y+(r->height*2/3),(int)r->width*6/7,(int)r->height/3));

        double t = (double)cvGetTickCount();
        CvSeq* faces = cvHaarDetectObjects( img, cascade, storage,
                                            1.1, min_neighbors, CV_HAAR_FIND_BIGGEST_OBJECT/*CV_HAAR_DO_CANNY_PRUNING*/,
                                            cvSize(22, 10) );
        t = (double)cvGetTickCount() - t;
        if (faces->total ==0) {
        } else {
          CvAvgComp* comp = (CvAvgComp*)cvGetSeqElem(faces, 0);
          CvRect* r = (CvRect*)cvGetSeqElem( faces, 0 );

          //comp->neighbors is equivalent to smile intensity
          fprintf(stdout,"%d %d %d %d %d",comp->neighbors,r->x+roi->x+(roi->width*1/5),r->y+roi->y+(roi->height*2/3),r->width,r->height);
          cvSetImageROI(img,cvRect(r->x+roi->x+(roi->width*1/5),r->y+roi->y+(roi->height*2/3),r->width,r->height));
          cvSaveImage("smile.jpg",img);
          cvResetImageROI(img);
        }
        fflush(stdout);
        for( i = 0; i < (faces ? faces->total : 0); i++ )
        {
            CvRect* f = (CvRect*)cvGetSeqElem( faces, i );
            cvRectangle(img,cvPoint(f->x,f->y),cvPoint(f->x+f->width,f->y+f->height),cvScalar(255,0,255,0),1,0,0);
            cvResetImageROI(img);
          }
    }

    if (show) cvShowImage( "result", img );
    cvReleaseImage( &gray );
}
