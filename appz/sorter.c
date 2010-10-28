#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cv.h>
#include <highgui.h>
#include <dirent.h>

using namespace std;


string filename;
string filename_neg;

struct dirent *dp;
bool boxed=false;

void RotationImage(IplImage* iBuf, IplImage** oBuf, int angle)
{
    IplImage* lTamp = cvCloneImage(iBuf);
        float lRotateValues[6];
        lRotateValues[0] = (float)(cos(angle*CV_PI/180.));
        lRotateValues[1] = (float)(sin(angle*CV_PI/180.));
        lRotateValues[3] = -lRotateValues[1];
        lRotateValues[4] =  lRotateValues[0];
        lRotateValues[2] = iBuf->width  *0.5f;  
        lRotateValues[5] = iBuf->height *0.5f; 
        CvMat lRotateMatrix = cvMat(2, 3, CV_32F, lRotateValues);
        cvGetQuadrangleSubPix(iBuf,lTamp,&lRotateMatrix); 
        *oBuf = lTamp;
}
                                            

// Define our callback which we will install for
// mouse events.
//
void my_mouse_callback(
 int event, int x, int y, int flags, void* param 
);

CvRect box;
bool drawing_box = false;

// A litte subroutine to draw a box onto an image
void draw_box( IplImage* img, CvRect rect ) {
 cvRectangle (
 img, 
 cvPoint(box.x,box.y),
 cvPoint(box.x+box.width,box.y+box.height),
 cvScalar(0xff,0x00,0x00) /* red */
 );
}

int main( int argc, char* argv[] ) {

 int offset;
 
 if (argv[4]) {
   offset=atoi(argv[4]);
  }
 
 if (argc<4) {
   fprintf(stderr, "Usage: sorter input_directory output_positive_example_directory output_negative_example_directory offset\n" );
   fprintf(stderr, "USE MOUSE TO DRAG & DROP THE REGION OF INTEREST, ROTATE THE PICTURE BY M / L KEYS\n");
   fprintf(stderr,"(use offset integer after interrupted session if You don't want to sort hundreds of files You have already sorted\n");
   fprintf(stderr,"RECOMMANDATION: the standard output of this application can be used to generate opencv-createsamples idx info files!\n");
   return -1;
 }
  
 box = cvRect(-1,-1,0,0);

 IplImage* image = cvCreateImage( 
 cvSize(200,200),
 IPL_DEPTH_32F,
 3
 );
 cvZero( image );
 IplImage* temp = cvCloneImage( image );

 cvNamedWindow( "Box Example" );
 cvSetMouseCallback("Box Example", my_mouse_callback, (void*) image );

 // The main program loop. 
  DIR *dir = opendir(argv[1]);
  fprintf(stderr,"ARGV %s \n",argv[1]);
  int i=0;
  while ((dp=readdir(dir)) != NULL) {
    //printf("%d\n",i);
    i++;
    if (i<offset) {
      continue;
    }
    std:ostringstream oss;
    oss << i;
    filename="";
    filename += oss.str();    
    filename +=".jpg";
    filename_neg="";
    filename_neg=argv[3]+filename;
    filename=argv[2]+filename;
    string rfilename=argv[1];
    rfilename+=dp->d_name;
    
    image=cvLoadImage(rfilename.c_str(),0); 
    if (!image) {
      //printf("Could not load image file: %s\n",dp->d_name);
      continue;
    }
    IplImage* temp = cvCloneImage( image );
    boxed=false;
    
     while( 1 ) {
       cvCopyImage( image, temp );
       if( drawing_box ) draw_box( temp, box ); 
       cvShowImage( "Box Example", temp );

       int c=cvWaitKey(15);   
      
       if (c==' ') {
         if (boxed) {
           printf("%s 1 %d %d %d %d \n",filename.c_str(), box.x, box.y, box.width, box.height);
           fflush(stdout);
           cvSaveImage(filename.c_str(),temp);
         }
         else {
           cvSaveImage(filename_neg.c_str(),temp);         
         }
         break;
       }
              
       switch(c) {
         case 'm': {
           RotationImage(temp,&image,12); 
           //boxed=true;
           break; 
          }
         case 'l': {
           RotationImage(temp,&image,-12); 
           //boxed=true;
           break; 
          }
      }
    }
  }
 cvReleaseImage( &image );
 cvReleaseImage( &temp );
 cvDestroyWindow( "Box Example" );
}

// mouse callback. If the user
// presses the left button, we start a box.
// when the user releases that button, then we
// add the box to the current image. When the
// mouse is dragged (with the button down) we 
// resize the box.
void my_mouse_callback(
 int event, int x, int y, int flags, void* param )
{

 IplImage* image = (IplImage*) param;

 switch( event ) {
 case CV_EVENT_MOUSEMOVE: {
 if( drawing_box ) {
 box.width = x-box.x;
 box.height = y-box.y;
 }
 }
 break;

 case CV_EVENT_LBUTTONDOWN: {
   drawing_box = true;
   box = cvRect( x, y, 0, 0 );
 }
 break; 

 case CV_EVENT_LBUTTONUP: {
   boxed=true;
   drawing_box = false; 
   if( box.width<0 ) { 
     box.x+=box.width; 
     box.width *=-1; 
   }
   if( box.height<0 ) { 
     box.y+=box.height; 
     box.height*=-1; 
   }
   draw_box( image, box );
  }
  break; 
}

}