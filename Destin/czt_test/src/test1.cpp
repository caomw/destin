#include "stdio.h"
#include "VideoSource.h"
#include "DestinNetworkAlt.h"
#include "Transporter.h"
#include "unit_test.h"
#include <time.h>
#include "macros.h"
//
#include "stereovision.h"
#include "stereocamera.h"
//
#include "ImageSourceImpl.h"
#include "czt_lib.h"
#include "CztMod.h"

#include "SomPresentor.h"
#include "ClusterSom.h"
#include <stdlib.h>
#include <vector>

using namespace cv;

// Helper function prototypes
double printFPS(bool print);
void combineWithDepth_1(float * fIn, int size, int extRatio, float * tempOut);
void combineWithDepth_2(float * fIn, float * depth, int size, float * tempOut);
IplImage * convert_c2(cv::Mat in);
void convert(cv::Mat & in, float * out);

// Prototypes of testing functions
void testOrg();
void testStep2();
void testStep3();
void test_CL2();
void test_CL_and_CL2();
void test_Update();
void test_RandomInput();
void test_SOM();
void test_SOM2();
void test_TempFunc();
void test_Quality();

// main function
int main(int argc, char ** argv){
    // Run the tests
    //testOrg();
    //testStep2();
    //testStep3();
    //test_CL2();
    //test_CL_and_CL2();
    //test_Update();
    //test_RandomInput();
    //test_SOM();
    test_SOM2();
    //test_TempFunc();
    //test_Quality();
}

/** meaures time between calls and prints the fps.
 *  print - prints the fps if true, otherwise just keeps the timer consistent
 */
double printFPS(bool print){
    // start = initial frame time
    // end = final frame time
    // sec = time count in seconds
    // set all to 0
    static double end, sec, start = 0;

    // set final time count to current tick count
    end = (double)cv::getTickCount();

    float out;
    //
    if(start != 0){
        sec = (end - start) / getTickFrequency();
        if(print==true){
            printf("fps: %f\n", 1 / sec);
        }
        out = 1/sec;
    }
    start = (double)cv::getTickCount();
    return out;
}

void combineWithDepth_1(float * fIn, int size, int extRatio, float * tempOut)
{
    int i,j;
    for(i=0; i<size; ++i)
    {
        tempOut[i] = fIn[i];
    }
    for(i=1; i<extRatio; ++i)
    {
        for(j=0; j<size; ++j)
        {
            tempOut[size*i+j] = (float)rand() / (float)RAND_MAX;
            //tempOut[size*i+j] = 0.5;
        }
    }
}

void combineWithDepth_2(float * fIn, float * depth, int size, float * tempOut)
{
    int i,j;
    for(i=0; i<size; ++i)
    {
        tempOut[i] = fIn[i];
    }
    for(i=0; i<size; ++i)
    {
        tempOut[size+i] = depth[i];
    }
}

IplImage * convert_c2(cv::Mat in)
{
    IplImage * out = new IplImage(in);
    IplImage * real_out;
    real_out = cvCreateImage(cvGetSize(out), IPL_DEPTH_8U, 1);
    cvCvtColor(out, real_out, CV_BGR2GRAY);
    return real_out;
}

void convert(cv::Mat & in, float * out) {
    if(in.channels()!=1){
        throw runtime_error("Excepted a grayscale image with one channel.");
    }
    if(in.depth()!=CV_8U){
        throw runtime_error("Expected image to have bit depth of 8bits unsigned integers ( CV_8U )");
    }
    cv::Point p(0, 0);
    int i = 0 ;
    for (p.y = 0; p.y < in.rows; p.y++) {
        for (p.x = 0; p.x < in.cols; p.x++) {
            //i = frame.at<uchar>(p);
            //use something like frame.at<Vec3b>(p)[channel] in case of trying to support color images.
            //There would be 3 channels for a color image (one for each of r, g, b)
            out[i] = (float)in.at<uchar>(p) / 255.0f;
            i++;
        }
    }
}

void testOrg(){
    // 2013.4.5
    // I want to test the original destin codes again and learn the CMake further!

    VideoSource vs(true, "");
    vs.enableDisplayWindow();

    SupportedImageWidths siw = W512;
    // Bottom to Top
    //uint centroid_counts[]  = {3,2,3,2,3,2,3,4};
    uint centroid_counts[]  = {4,3,5,3,3,2,3,4};
    bool isUniform = true;

    DestinNetworkAlt * network = new DestinNetworkAlt(siw, 8, centroid_counts, isUniform);

    Transporter t;
    vs.grab();//throw away first frame in case its garbage
    int frameCount = 0;
    double totalFps = 0.0;
    while(vs.grab()){
        frameCount++;

        t.setSource(vs.getOutput());
        t.transport(); //move video from host to card

        network->doDestin(t.getDest());

        if(frameCount % 2 != 0 ){ //only print every 2rd so display is not so jumpy
            totalFps += printFPS(false);
            continue;
        }

        printf("\033[2J\033[1;1H");
        printf("----------------TEST_ORG----------------\n");

        printf("Frame: %i\n", frameCount);
        totalFps += printFPS(true);
        printf("Average FPS now: %f\n", totalFps/frameCount);

        int layer = 7;
        Node & n = *network->getNode(layer,0,0);
        printf("Node %i,0,0 winner: %i\n",layer, n.winner);
        printf("Node centroids: %i\n", n.nb);

        printf("Node starv:");
        printFloatArray(n.starv, n.nb);
        printf("Starv coef: %f \n", n.starvCoeff);
        printf("\n");


        //printf("layer %i node 0 centroid locations:\n", layer);
        //network->printNodeCentroidPositions(layer, 0, 0);
        for(int l = 0 ; l < 8 ; l++){
            printf("belief graph layer: %i\n",l);
            network->printBeliefGraph(l,0,0);
        }
    }
    delete network;
}

/*void testStep2(){
    // 2013.7.5
    // TODO: the following codes could be referred as how to use 2-webcam input;
    // could be removed sometime;

    VideoSource vs1(true, "", CV_CAP_ANY);
    VideoSource vs2(true, "", CV_CAP_ANY+1);
    vs1.enableDisplayWindow("left");
    vs2.enableDisplayWindow("right");
    vs1.grab();
    vs2.grab();

    int result;
    StereoVision * sv = new StereoVision(640, 480);
    //StereoVision * sv = new StereoVision(512, 512);
    result = sv->calibrationLoad("calibration.dat");
    printf("calibrationLoad() status: %d\n", result);
    CvSize imageSize = sv->getImageSize();
    CvMat * imageRectifiedPair = cvCreateMat( imageSize.height, imageSize.width*2,CV_8UC3 );
    IplImage * img_l;
    IplImage * img_r;
    float * float_l, * float_r, * float_depth, * float_combined;
    MALLOC(float_l, float, 512*512);
    MALLOC(float_r, float, 512*512);
    MALLOC(float_depth, float, 512*512);
    MALLOC(float_combined, float, 512*512*2);

    SupportedImageWidths siw = W512;
    uint centroid_counts[]  = {4,3,5,3,3,2,3,3};
    bool isUniform = true;
    int extRatio = 2;
    DestinNetworkAlt * network = new DestinNetworkAlt(siw, 8, centroid_counts, isUniform, extRatio);
    int frameCount = 0;
    double totalFps = 0.0;

    // Method 1
    // Should use 'VideoSource' part!!!
    //
    while(vs1.grab() && vs2.grab())
    {
        frameCount++;

        img_l = convert_c2(vs1.getOutput_c1());
        img_r = convert_c2(vs2.getOutput_c1());
        result = sv->stereoProcess(img_l, img_r);
        cvShowImage("depth", sv->imageDepthNormalized_c1);
        //printf("%d * %d\n", vs1.getOutput_c1().rows, vs1.getOutput_c1().cols);
        //printf("%d * %d\n", img_l->width, img_l->height);
        //printf("%d * %d\n", sv->imageDepthNormalized->width, sv->imageDepthNormalized->height);

        //CvMat part;
        //cvGetCols( imageRectifiedPair, &part, 0, imageSize.width );
        //cvCvtColor( sv->imagesRectified[0], &part, CV_GRAY2BGR );
        //cvGetCols( imageRectifiedPair, &part, imageSize.width,imageSize.width*2 );
        //cvCvtColor( sv->imagesRectified[1], &part, CV_GRAY2BGR );
        //for(int j = 0; j < imageSize.height; j += 16 )
        //    cvLine( imageRectifiedPair, cvPoint(0,j),cvPoint(imageSize.width*2,j),CV_RGB((j%3)?0:255,((j+1)%3)?0:255,((j+2)%3)?0:255));
        //cvShowImage( "rectified", imageRectifiedPair );

        float_l = vs1.getOutput();
        float_r = vs2.getOutput();
        cv::Mat tempMat(sv->imageDepthNormalized_c1);
        convert(tempMat, float_depth);
        combineWithDepth_2(float_l, float_depth, 512*512, float_combined);

        network->doDestin(float_combined);
        //network->doDestin_c1(float_l);    // extRatio should be 1 for this!

        if(frameCount % 2 != 0 ){ //only print every 2rd so display is not so jumpy
            totalFps += printFPS(false);
            continue;
        }

        // Old clear screen method
        //printf("\033[2J");

        // New clear screen method (might give less flickering...?)
        printf("\033[2J\033[1;1H");
        printf("----------------TEST_ORG----------------\n");

        printf("Frame: %i\n", frameCount);
        totalFps += printFPS(true);
        printf("Average FPS now: %f\n", totalFps/frameCount);
        //int layer = 1;
        // CZT
        //
        int layer = 7;
        Node & n = *network->getNode(layer,0,0);
        printf("Node %i,0,0 winner: %i\n",layer, n.winner);
        printf("Node centroids: %i\n", n.nb);

        printf("Node starv:");
        printFloatArray(n.starv, n.nb);
        printf("Starv coef: %f \n", n.starvCoeff);
        printf("\n");

        // CZT
        //
        //printf("layer %i node 0 centroid locations:\n", layer);
        //network->printNodeCentroidPositions(layer, 0, 0);
        for(int l = 0 ; l < 8 ; l++){
            printf("belief graph layer: %i\n",l);
            network->printBeliefGraph(l,0,0);
        }
    }

    // Method 2
    // Should comment the 'VideoSourece' Part!!! Here we use 'StereoCamera' class instead!!!
    //
    StereoCamera camera;
    if(camera.setup(imageSize) == 0)
    {
        while(true)
        {
            camera.capture();
            img_l = camera.getFramesGray(0);
            img_r = camera.getFramesGray(1);
            cvShowImage("left", img_l);
            cvShowImage("right", img_r);

            sv->stereoProcess(camera.getFramesGray(0), camera.getFramesGray(1));
            cvShowImage("depth", sv->imageDepthNormalized);

            CvMat part;
            cvGetCols( imageRectifiedPair, &part, 0, imageSize.width );
            cvCvtColor( sv->imagesRectified[0], &part, CV_GRAY2BGR );
            cvGetCols( imageRectifiedPair, &part, imageSize.width,imageSize.width*2 );
            cvCvtColor( sv->imagesRectified[1], &part, CV_GRAY2BGR );
            for(int j = 0; j < imageSize.height; j += 16 )
                cvLine( imageRectifiedPair, cvPoint(0,j),cvPoint(imageSize.width*2,j),CV_RGB((j%3)?0:255,((j+1)%3)?0:255,((j+2)%3)?0:255));
            cvShowImage( "rectified", imageRectifiedPair );

            cvWaitKey(10);
        }
    }

    cvReleaseImage(&img_l);
    cvReleaseImage(&img_r);
    cvReleaseMat(&imageRectifiedPair);
    FREE(sv);
    FREE(float_l);
    FREE(float_r);
    FREE(float_depth);
    FREE(float_combined);
    delete network;
}*/

void testStep3(){
    // 2013.7.5
    // CZT: to process BGR, 1-webcam video input;

    VideoSource vs(true, "");
    vs.enableDisplayWindow();
    vs.turnOnColor();
    CztMod * cl2 = new CztMod();

    SupportedImageWidths siw = W512;
    uint centroid_counts[]  = {4,3,5,3,3,2,3,4};
    bool isUniform = true;
    int nLayers = 8;
    int size = 512*512;
    int extRatio = 3;

    DestinNetworkAlt * network = new DestinNetworkAlt(siw, nLayers, centroid_counts, isUniform, extRatio);
    float * tempIn;
    MALLOC(tempIn, float, size*extRatio);

    Transporter t;
    vs.grab();//throw away first frame in case its garbage
    int frameCount = 0;

    double totalFps = 0.0;
    while(vs.grab()){
        frameCount++;
        cl2->combineBGR(vs.getBFrame(), vs.getGFrame(), vs.getRFrame(), size, tempIn);
        network->doDestin(tempIn);

        if(frameCount % 2 != 0 ){ //only print every 2rd so display is not so jumpy
            totalFps += printFPS(false);
            continue;
        }

        // New clear screen method (might give less flickering...?)
        printf("\033[2J\033[1;1H");
        printf("----------------TEST_ORG----------------\n");

        printf("Frame: %i\n", frameCount);
        totalFps += printFPS(true);
        printf("Average FPS now: %f\n", totalFps/frameCount);

        int layer = 7;
        Node & n = *network->getNode(layer,0,0);
        printf("Node %i,0,0 winner: %i\n",layer, n.winner);
        printf("Node centroids: %i\n", n.nb);

        printf("Node starv:");
        printFloatArray(n.starv, n.nb);
        printf("Starv coef: %f \n", n.starvCoeff);
        printf("\n");

        for(int l = 0 ; l < 8 ; l++){
            printf("belief graph layer: %i\n",l);
            network->printBeliefGraph(l,0,0);
        }
    }
    delete network;
    FREE(tempIn);
}

void test_CL2()
{
    // Add Random depth information
    // Test czt_lib2 (which is my own library of functions!)

    CztMod * cl2 = new CztMod();
    ImageSouceImpl isi;
    isi.addImage("/home/teaera/Downloads/destin_toshare/train images/A.png");

    SupportedImageWidths siw = W512;
    uint centroid_counts[]  = {4,8,16,32,64,32,16,8};
    bool isUniform = true;
    int size = 512*512;
    int extRatio = 2;
    DestinNetworkAlt * network = new DestinNetworkAlt(siw, 8, centroid_counts, isUniform, extRatio);

    int inputSize = size*extRatio;
    float * tempIn;
    MALLOC(tempIn, float, inputSize);

    int frameCount = 1;
    int maxCount = 3000;
    while(frameCount <= maxCount){
        frameCount++;
        if(frameCount % 10 == 0)
        {
            printf("Count %d;\n", frameCount);
        }

        isi.findNextImage();
        cl2->combineInfo_extRatio(isi.getGrayImageFloat(), size, extRatio, tempIn);
        //network->doDestin(isi.getGrayImageFloat());
        network->doDestin(tempIn);
    }

    network->displayLayerCentroidImages(7, 1000);
    cv::waitKey(10000);
    network->saveLayerCentroidImages(7, "/home/teaera/Pictures/2013.7.3_A_level7.jpg");
    network->saveLayerCentroidImages(6, "/home/teaera/Pictures/2013.7.3_A_level6.jpg");
    network->saveLayerCentroidImages(5, "/home/teaera/Pictures/2013.7.3_A_level5.jpg");
    network->saveLayerCentroidImages(4, "/home/teaera/Pictures/2013.7.3_A_level4.jpg");
    network->saveLayerCentroidImages(3, "/home/teaera/Pictures/2013.7.3_A_level3.jpg");
    network->saveLayerCentroidImages(2, "/home/teaera/Pictures/2013.7.3_A_level2.jpg");
    network->saveLayerCentroidImages(1, "/home/teaera/Pictures/2013.7.3_A_level1.jpg");
    network->saveLayerCentroidImages(0, "/home/teaera/Pictures/2013.7.3_A_level0.jpg");

    delete network;
}

void test_CL_and_CL2()
{
    // This is to test the combined information and show the centroids for combined
    // information!

    CztMod * cl2 = new CztMod();
    czt_lib * cl = new czt_lib();

    SupportedImageWidths siw = W512;
    uint centroid_counts[]  = {4,8,16,32,64,32,16,8};
    bool isUniform = true;
    int size = 512*512;
    int extRatio = 2;
    DestinNetworkAlt * network = new DestinNetworkAlt(siw, 8, centroid_counts, isUniform, extRatio);

    int inputSize = size*extRatio;
    float * tempIn1, * tempIn2, * tempIn;
    MALLOC(tempIn1, float, size);
    MALLOC(tempIn2, float, size);
    MALLOC(tempIn, float, inputSize);
    cl->isNeedResize("/home/teaera/Work/RECORD/2013.5.8/pro_3/1.jpg");
    tempIn1 = cl->get_float512();
    cl->isNeedResize("/home/teaera/Work/RECORD/2013.5.8/pro_add_3/1.jpg");
    tempIn2 = cl->get_float512();
    cl2->combineInfo_depth(tempIn1, tempIn2, size, tempIn);

    int frameCount = 1;
    int maxCount = 3000;
    while(frameCount <= maxCount){
        frameCount++;
        if(frameCount % 10 == 0)
        {
            printf("Count %d;\n", frameCount);
        }

        network->doDestin(tempIn);
    }

    network->displayLayerCentroidImages(7, 1000);
    cv::waitKey(10000);
    network->saveLayerCentroidImages(7, "/home/teaera/Pictures/2013.5.13_level7.jpg");
    network->saveLayerCentroidImages(6, "/home/teaera/Pictures/2013.5.13_level6.jpg");
    network->saveLayerCentroidImages(5, "/home/teaera/Pictures/2013.5.13_level5.jpg");
    network->saveLayerCentroidImages(4, "/home/teaera/Pictures/2013.5.13_level4.jpg");
    network->saveLayerCentroidImages(3, "/home/teaera/Pictures/2013.5.13_level3.jpg");
    network->saveLayerCentroidImages(2, "/home/teaera/Pictures/2013.5.13_level2.jpg");
    network->saveLayerCentroidImages(1, "/home/teaera/Pictures/2013.5.13_level1.jpg");
    network->saveLayerCentroidImages(0, "/home/teaera/Pictures/2013.5.13_level0.jpg");

    delete network;
}

void test_Update()
{
//#define TEST_ADD
#define RUN_BEFORE
#define RUN_NOW
#define SHOW_BEFORE
#define SHOW_NOW
//#define TEST_nb
//#define TEST_uf_persistWinCounts
//#define TEST_uf_persistWinCounts_detailed
//#define TEST_uf_avgDelta  //uf_sigma
//#define TEST_mu
//#define TEST_observation
//#define TEST_beliefMal

    ImageSouceImpl isi;
    isi.addImage("/home/teaera/Downloads/destin_toshare/train images/A.png");
    //isi.addImage("/home/teaera/Work/RECORD/2013.5.8/pro_1/3.jpg");
    CztMod * cl2 = new CztMod();
    // currLayer: the layer, in which you want to add centroids or kill centroids;
    // tempLayer: for backup;
    uint tempLayer;
    uint currLayer = 0;
    uint kill_ind = 0;
#define TEST_layer0
#define TEST_layer7

    SupportedImageWidths siw = W512;
    uint nLayer = 8;
    //uint centroid_counts[]  = {1,8,16,32,32,16,8,4}; // For adding
    uint centroid_counts[]  = {4,8,16,32,32,16,8,4}; // For killing
                                                     // HumanFace_1500_case1
    //uint centroid_counts[]  = {8,16,16,32,32,16,8,4}; // HumanFace_1500_case2
    //uint centroid_counts[]  = {2,3,4,5,4,3,2,1};
    //uint centroid_counts[]  = {6,8,10,12,12,8,6,4};
    //uint centroid_counts[]  = {4,4,4,4,4,4,4,4};
    bool isUniform = true;

    /*int size = 512*512;
    int extRatio = 2;
    float * tempIn;
    MALLOC(tempIn, float, size*extRatio);*/

    DestinNetworkAlt * network = new DestinNetworkAlt(siw, nLayer, centroid_counts, isUniform);


    int frameCount;
    int maxCount = 10;

    int i, l, j;
    Destin * d = network->getNetwork();

#ifdef RUN_BEFORE
    frameCount = 1;
    while(frameCount <= maxCount){
        frameCount++;
        if(frameCount % 10 == 0)
        {
            printf("Count %d;\n", frameCount);
            for(l=0; l<d->nLayers; ++l)
            {
                printf("Layer %d\n", l);
                for(i=0; i<d->nb[l]; ++i)
                {
                    // Only display the 'ni' part!
                    for(j=0; j<network->getNode(l,0,0)->ni; ++j)
                    {
                        //printf("%f  ", d->uf_avgDelta[l][i*network->getNode(l,0,0)->ns+j]);
                        //printf("%f  ", d->uf_sigma[l][i*network->getNode(l,0,0)->ns+j]);
                        printf("%f  ", d->uf_absvar[l][i*network->getNode(l,0,0)->ns+j]);
                    }
                    printf("\n");
                }
                printf("------\n");
            }
            printf("\n");/**/
        }

        isi.findNextImage();
        /*cl2->combineInfo_extRatio(isi.getGrayImageFloat(), size, extRatio, tempIn);
        network->doDestin(tempIn);*/
        network->doDestin(isi.getGrayImageFloat());
    }
#endif // RUN_BEFORE

#ifdef SHOW_BEFORE
    tempLayer = 7;
    //tempLayer = currLayer;
    network->displayLayerCentroidImages(tempLayer, 1000);
    cv::waitKey(3000);
    network->saveLayerCentroidImages(tempLayer, "/home/teaera/Pictures/2013.6.10_nm_before.jpg");
#endif // SHOW_BEFORE

    Node * node1 = network->getNode(currLayer, 0, 0);
#ifndef TEST_layer0
    Node * node2 = network->getNode(currLayer-1, 0, 0);
#endif
#ifndef TEST_layer7
    Node * node3 = network->getNode(currLayer+1, 0, 0);
#endif

#ifdef TEST_nb
    printf("------------TEST_nb\n");
    for(i=0; i<d->nLayers; ++i)
    {
        Node * tempNode = network->getNode(i, 0, 0);
        printf("Layer %d\n", i);
        printf("%d  %d  %d  %d  %d\n", d->nb[i], tempNode->ni, tempNode->nb, tempNode->np, tempNode->ns);
        printf("------\n");
    }
    printf("\n");
#endif // TEST_nb

#ifdef TEST_uf_persistWinCounts
    printf("------------TEST_uf_persistWinCounts\n");
    for(l=0; l<d->nLayers; ++l)
    {
        printf("Layer %d\n", l);
        for(i=0; i<d->nb[l]; ++i)
        {
            printf("%ld  ", d->uf_persistWinCounts[l][i]);
            // uf_persistWinCounts, long;
            // uf_starv, float;
            // uf_winCounts, uint;
        }
        printf("\n------\n");
    }
    printf("\n");
#endif // TEST_uf_persistWinCounts

#ifdef TEST_uf_persistWinCounts_detailed
    printf("------------TEST_uf_persistWinCounts_detailed\n");
    for(l=0; l<d->nLayers; ++l)
    {
        printf("Layer %d\n", l);
        for(i=0; i<d->nb[l]; ++i)
        {
            printf("%ld  ", d->uf_persistWinCounts_detailed[l][i]);
        }
        printf("\n------\n");
    }
    printf("\n");
#endif // TEST_uf_persistWinCounts_detailed

#ifdef TEST_uf_avgDelta
    printf("------------TEST_uf_avgDelt\n");
    for(l=0; l<d->nLayers; ++l)
    {
        printf("Layer %d\n", l);
        for(i=0; i<d->nb[l]; ++i)
        {
            for(j=0; j<network->getNode(l,0,0)->ns; ++j)
            {
                printf("%f  ", d->uf_avgDelta[l][i*network->getNode(l,0,0)->ns+j]);
                //printf("%f  ", d->uf_sigma[l][i*network->getNode(l,0,0)->ns+j]);
                //printf("%f  ", d->uf_absvar[l][i*network->getNode(l,0,0)->ns+j]);
            }
            printf("\n");
        }
        printf("------\n");
    }
    printf("\n");
#endif // TEST_uf_avgDelta

#ifdef TEST_mu
    printf("------------TEST_mu\n");
    printf("------Node: %d,0,0\n", currLayer);
    for(i=0; i<node1->nb; ++i)
    {
        for(j=0; j<node1->ns; ++j)
        {
            printf("%f  ", node1->mu[i*node1->ns+j]);
        }
        printf("\n---\n");
    }
#ifndef TEST_layer0
    printf("------Node: %d,0,0\n", currLayer-1);
    for(i=0; i<node2->nb; ++i)
    {
        for(j=0; j<node2->ns; ++j)
        {
            printf("%f  ", node2->mu[i*node2->ns+j]);
        }
        printf("\n---\n");
    }
#endif
#ifndef TEST_layer7
    printf("------Node: %d,0,0\n", currLayer+1);
    for(i=0; i<node3->nb; ++i)
    {
        for(j=0; j<node3->ns; ++j)
        {
            printf("%f  ", node3->mu[i*node3->ns+j]);
        }
        printf("\n---\n");
    }
#endif
    printf("\n");
#endif // TEST_mu

#ifdef TEST_observation
    printf("------------TEST_observation\n");
    printf("------Node: %d,0,0\n", currLayer);
    for(i=0; i<node1->nb; ++i)
    {
        for(j=0; j<node1->ns; ++j)
        {
            printf("%f  ", node1->observation[i*node1->ns+j]);
        }
        printf("\n---\n");
    }
#ifndef TEST_layer0
    printf("------Node: %d,0,0\n", currLayer-1);
    for(i=0; i<node2->nb; ++i)
    {
        for(j=0; j<node2->ns; ++j)
        {
            printf("%f  ", node2->observation[i*node2->ns+j]);
        }
        printf("\n---\n");
    }
#endif
#ifndef TEST_layer7
    printf("------Node: %d,0,0\n", currLayer+1);
    for(i=0; i<node3->nb; ++i)
    {
        for(j=0; j<node3->ns; ++j)
        {
            printf("%f  ", node3->observation[i*node3->ns+j]);
        }
        printf("\n---\n");
    }
#endif
    printf("\n");
#endif // TEST_observation

#ifdef TEST_beliefMal
    printf("------------TEST_beliefMal\n");
    printf("------Node: %d,0,0\n", currLayer);
    for(i=0; i<node1->nb; ++i)
    {
        printf("%f  ", node1->beliefMal[i]);
        // node1->beliefMal
        // node1->beliefEuc
    }
    printf("\n");
#ifndef TEST_layer0
    printf("------Node: %d,0,0\n", currLayer-1);
    for(i=0; i<node2->nb; ++i)
    {
        printf("%f  ", node2->beliefMal[i]);
        // node1->beliefMal
        // node1->beliefEuc
    }
    printf("\n");
#endif
#ifndef TEST_layer7
    printf("------Node: %d,0,0\n", currLayer+1);

    delete network;
    for(i=0; i<node3->nb; ++i)
    {
        printf("%f  ", node3->beliefMal[i]);
        // node1->beliefMal
        // node1->beliefEuc
    }
    printf("\n");
#endif
    printf("\n");
#endif // TEST_beliefMal

//---------------------------------------------------------------------------//
    printf("--------------------------------------------------------------\n\n");
//---------------------------------------------------------------------------//

#ifdef TEST_ADD
    // Add
    /*centroid_counts[currLayer]++;
    network->updateDestin_add(siw, 8, centroid_counts, isUniform, size, extRatio, currLayer);
    centroid_counts[currLayer]++;
    network->updateDestin_add(siw, 8, centroid_counts, isUniform, size, extRatio, currLayer);
    centroid_counts[currLayer]++;
    network->updateDestin_add(siw, 8, centroid_counts, isUniform, size, extRatio, currLayer);*/

    // Kill
    centroid_counts[currLayer]--;
    network->updateDestin_kill(siw, 8, centroid_counts, isUniform, size, extRatio, currLayer, kill_ind);
    centroid_counts[currLayer]--;
    network->updateDestin_kill(siw, 8, centroid_counts, isUniform, size, extRatio, currLayer, kill_ind);
    centroid_counts[currLayer]--;
    network->updateDestin_kill(siw, 8, centroid_counts, isUniform, size, extRatio, currLayer, kill_ind);/**/

#ifdef RUN_NOW
    frameCount = 1;
    while(frameCount <= maxCount){
        frameCount++;
        if(frameCount % 10 == 0)
        {
            printf("Count %d;\n", frameCount);
        }

        isi.findNextImage();
        cl2->combineInfo_extRatio(isi.getGrayImageFloat(), size, extRatio, tempIn);
        network->doDestin(tempIn);
        //network->doDestin(isi.getGrayImageFloat());
    }
#endif // RUN_NOW

#ifdef SHOW_NOW
    tempLayer = 7;
    //tempLayer = currLayer;
    network->displayLayerCentroidImages(tempLayer, 1000);
    cv::waitKey(3000);
    network->saveLayerCentroidImages(tempLayer, "/home/teaera/Pictures/2013.6.10_nm_now.jpg");
#endif // SHOW_NOW

    // I don't why I shoud 'reload' these nodes again?
    // Otherwise, the display of node->mu will have some problems?
    // Maybe my fault somewhere?
    //
    node1 = network->getNode(currLayer, 0, 0);
#ifndef TEST_layer0
    node2 = network->getNode(currLayer-1, 0, 0);
#endif
#ifndef TEST_layer7
    node3 = network->getNode(currLayer+1, 0, 0);
#endif

#ifdef TEST_nb
    printf("------TEST_nb\n");
    for(i=0; i<d->nLayers; ++i)
    {
        Node * tempNode = network->getNode(i, 0, 0);
        printf("Layer %d\n", i);
        printf("%d  %d  %d  %d  %d\n", d->nb[i], tempNode->ni, tempNode->nb, tempNode->np, tempNode->ns);
        printf("------\n");
    }
    printf("\n");
#endif // TEST_nb

#ifdef TEST_uf_persistWinCounts
    printf("------TEST_uf_persistWinCounts\n");
    for(l=0; l<d->nLayers; ++l)
    {
        printf("Layer %d\n", l);
        for(i=0; i<d->nb[l]; ++i)
        {
            printf("%ld  ", d->uf_persistWinCounts[l][i]);
            // uf_persistWinCounts, long;
            // uf_starv, float;
            // uf_winCounts, uint;
        }
        printf("\n------\n");
    }
    printf("\n");
#endif // TEST_uf_persistWinCounts

#ifdef TEST_uf_persistWinCounts_detailed
    printf("------------TEST_uf_persistWinCounts_detailed\n");
    for(l=0; l<d->nLayers; ++l)
    {
        printf("Layer %d\n", l);
        for(i=0; i<d->nb[l]; ++i)
        {
            printf("%ld  ", d->uf_persistWinCounts_detailed[l][i]);
        }
        printf("\n------\n");
    }
    printf("\n");
#endif // TEST_uf_persistWinCounts_detailed

#ifdef TEST_uf_avgDelta
    printf("------------TEST_uf_avgDelt\n");
    for(l=0; l<d->nLayers; ++l)
    {
        printf("Layer %d\n", l);
        for(i=0; i<d->nb[l]; ++i)
        {
            for(j=0; j<network->getNode(l,0,0)->ns; ++j)
            {
                //printf("%f  ", d->uf_avgDelta[l][i*network->getNode(l,0,0)->ns+j]);
                printf("%e  ", d->uf_sigma[l][i*network->getNode(l,0,0)->ns+j]);
            }
            printf("\n");
        }
        printf("------\n");
    }
    printf("\n");
#endif // TEST_uf_avgDelta

#ifdef TEST_mu
    printf("------------TEST_mu\n");
    printf("------Node: %d,0,0\n", currLayer);
    for(i=0; i<node1->nb; ++i)
    {
        for(j=0; j<node1->ns; ++j)
        {
            printf("%f  ", node1->mu[i*node1->ns+j]);
        }
        printf("\n---\n");
    }
#ifndef TEST_layer0
    printf("------Node: %d,0,0\n", currLayer-1);
    for(i=0; i<node2->nb; ++i)
    {
        for(j=0; j<node2->ns; ++j)
        {
            printf("%f  ", node2->mu[i*node2->ns+j]);
        }
        printf("\n---\n");
    }
#endif
#ifndef TEST_layer7
    printf("------Node: %d,0,0\n", currLayer+1);
    for(i=0; i<node3->nb; ++i)
    {
        for(j=0; j<node3->ns; ++j)
        {
            printf("%f  ", node3->mu[i*node3->ns+j]);
        }
        printf("\n---\n");
    }
#endif
    printf("\n");
#endif // TEST_mu

#ifdef TEST_observation
    printf("------------TEST_observation\n");
    printf("------Node: %d,0,0\n", currLayer);
    for(i=0; i<node1->nb; ++i)
    {
        for(j=0; j<node1->ns; ++j)
        {
            printf("%f  ", node1->observation[i*node1->ns+j]);
        }
        printf("\n---\n");
    }
#ifndef TEST_layer0
    printf("------Node: %d,0,0\n", currLayer-1);
    for(i=0; i<node2->nb; ++i)
    {
        for(j=0; j<node2->ns; ++j)
        {
            printf("%f  ", node2->observation[i*node2->ns+j]);
        }
        printf("\n---\n");
    }
#endif
#ifndef TEST_layer7
    printf("------Node: %d,0,0\n", currLayer+1);
    for(i=0; i<node3->nb; ++i)
    {
        for(j=0; j<node3->ns; ++j)
        {
            printf("%f  ", node3->observation[i*node3->ns+j]);
        }
        printf("\n---\n");
    }
#endif
    printf("\n");
#endif // TEST_observation

#ifdef TEST_beliefMal
    printf("------------TEST_beliefMal\n");
    printf("------Node: %d,0,0\n", currLayer);
    for(i=0; i<node1->nb; ++i)
    {
        printf("%f  ", node1->beliefMal[i]);
        // node1->beliefMal
        // node1->beliefEuc
    }
    printf("\n");
#ifndef TEST_layer036392cc2f513670c3f9e28804199602fd8666218
    printf("------Node: %d,0,0\n", currLayer-1);
    for(i=0; i<node2->nb; ++i)
    {
        printf("%f  ", node2->beliefMal[i]);
        // node1->beliefMal
        // node1->beliefEuc
    }
    printf("\n");
#endif
#ifndef TEST_layer7
    printf("------Node: %d,0,0\n", currLayer+1);
    for(i=0; i<node3->nb; ++i)
    {
        printf("%f  ", node3->beliefMal[i]);
        // node1->beliefMal
        // node1->beliefEuc
    }
    printf("\n");
#endif
    printf("\n");
#endif // TEST_beliefMal

#endif // TEST_add

    delete network;
}

void test_RandomInput()
{
    SupportedImageWidths siw = W512;
    uint nLayer = 8;
    uint centroid_counts[]  = {4,4,4,4,4,4,4,4};
    bool isUniform = true;
    DestinNetworkAlt * network = new DestinNetworkAlt(siw, nLayer, centroid_counts, isUniform);

    int size = 512*512;
    CztMod * cl2 = new CztMod();
    float * inArr = cl2->floatArrCreate(size);
    cl2->floatArrRandomize(inArr, size);
    cv::Mat inMat(512, 512, CV_8UC1);
    cl2->convert(inMat, inArr);
    cv::namedWindow("Test");
    cv::imshow("Test", inMat);
    cv::waitKey(5);

    uint frameCount = 1;
    uint maxCount = 1500;
    double totalFps = 0.0;
    while(frameCount <= maxCount || true){
        frameCount++;

        network->doDestin(inArr);
        cl2->floatArrRandomize(inArr, size);
        cl2->convert(inMat, inArr);
        cv::imshow("Test", inMat);
        cv::waitKey(5);

        if(frameCount % 2 != 0 ){ //only print every 2rd so display is not so jumpy
            totalFps += printFPS(false);
            continue;
        }

        printf("\033[2J\033[1;1H");
        printf("Frame: %i\n", frameCount);
        totalFps += printFPS(true);
        printf("Average FPS now: %f\n", totalFps/frameCount);

        int layer = 7;
        Node & n = *network->getNode(layer,0,0);
        printf("Node %i,0,0 winner: %i\n",layer, n.winner);
        printf("Node centroids: %i\n", n.nb);

        printf("Node starv:");
        printFloatArray(n.starv, n.nb);
        printf("Starv coef: %f \n", n.starvCoeff);
        printf("\n");

        //printf("layer %i node 0 centroid locations:\n", layer);
        //network->printNodeCentroidPositions(layer, 0, 0);
        for(int l = 0 ; l < 8 ; l++){
            printf("belief graph layer: %i\n",l);
            network->printBeliefGraph(l,0,0);
        }/**/
    }

    free(inArr);
    delete network;
}

void test_SOM()
{
    ImageSouceImpl isi;
    string imgs = "ABCDEFGHIJKLMNYZ", img;
    uint nImgs = imgs.length();
    for(int i=0; i<nImgs; ++i)
    {
        img = "";
        img.insert(img.begin(), imgs[i]);
        isi.addImage("/home/teaera/Work/RECORD/2013.7.22/32/" + img + ".png");
    }

    SupportedImageWidths siw = W32;
    uint nLayer = 4;
    //uint centroid_counts[]  = {96, 64, 32, 16};
    uint centroid_counts[]  = {64, 64, 32, 16};
    bool isUniform = true;
    DestinNetworkAlt * network = new DestinNetworkAlt(siw, nLayer, centroid_counts, isUniform);

    int frameCount = 1, maxCount = 1600;
    while(frameCount <= maxCount){
        frameCount++;
        if(frameCount % 10 == 0)
        {
            printf("Count %d;\n", frameCount);
        }

        isi.findNextImage();
        network->doDestin(isi.getGrayImageFloat());
    }

    for(int i=0; i<nLayer; ++i)
    {
        network->setLayerIsTraining(i, false);
    }

    // For testing
    for(int currLayer=0; currLayer<nLayer; ++currLayer)
    {
        Node * currNode = network->getNode(currLayer, 0, 0);
        uint nb = currNode->nb;
        uint dim = currNode->ni;
        printf("---Layer %d---\n", currLayer);
        printf("Centroids: %d; Dimensions:  %d;\n", nb, dim);
        uint som_rows = 200, som_cols = 200;
        uint som_train_iterations = 5000;

        ClusterSom som(som_rows, som_cols, dim);
        SomPresentor sp(som);

        // The centroids
        std::vector<std::vector<float> > muData;
        for(int i=0; i<nb; ++i)
        {
            std::vector<float> d;
            for(int j=0; j<dim; ++j)
            {
                d.push_back(currNode->mu[i*currNode->ns + j]);
            }
            muData.push_back(d);
            som.addTrainData(d.data());
        }
        printf("muData: %ld;\n", muData.size());

        // The observations
        std::vector<std::vector<float> > obsData;
        int currSize = network->getNetwork()->layerSize[currLayer];
        for(int idxImg=0; idxImg<nImgs; ++idxImg)
        {
            isi.findNextImage();
            float * f = isi.getGrayImageFloat();
            // In case
            for(int i=0; i<nLayer*10; ++i)
            {
                network->doDestin(f);
            }

            if(currLayer == 0)
            {
                int currWidth = (int)sqrt(currSize);
                for(int row=0; row<currWidth; ++row)
                {
                    for(int col=0; col<currWidth; ++col)
                    {
                        Node * tNode = network->getNode(currLayer, row, col);
                        std::vector<float> d;
                        for(int i=0; i<tNode->ni; ++i)
                        {
                            d.push_back(f[tNode->inputOffsets[i]]);
                        }
                        obsData.push_back(d);
                        som.addTrainData(d.data());
                    }
                }
            }
            else
            {
                int currWidth = (int)sqrt(currSize);
                for(int row=0; row<currWidth; ++row)
                {
                    for(int col=0; col<currWidth; ++col)
                    {
                        Node * tNode = network->getNode(currLayer, row, col);
                        Node ** childNodes = tNode->children;
                        std::vector<float> d;
                        for(int i=0; i<4; ++i)
                        {
                            for(int j=0; j<childNodes[i]->nb; ++j)
                            {
                                d.push_back(childNodes[i]->beliefMal[j]);
                            }
                        }
                        obsData.push_back(d);
                        som.addTrainData(d.data());
                    }
                }
            }
        }
        printf("obsData: %ld;\n", obsData.size());

        som.train(som_train_iterations);

        for(int i=0; i<muData.size(); ++i)
        {
            std::vector<float> d = muData.at(i);
            CvPoint cp = som.findBestMatchingUnit(d.data());
            sp.addSimMapMaker(cp.y, cp.x, 0.1, 5);
        }

        for(int i=0; i<obsData.size(); ++i)
        {
            std::vector<float> d = obsData.at(i);
            CvPoint cp = som.findBestMatchingUnit(d.data());
            sp.addSimMapMaker(cp.y, cp.x, 0.5, 3);
        }

        string filename = "/home/teaera/Pictures/";
        switch(currLayer)
        {
        case 0:
            filename += "layer0.jpg";
            break;
        case 1:
            filename += "layer1.jpg";
            break;
        case 2:
            filename += "layer2.jpg";
            break;
        case 3:
            filename += "layer3.jpg";
            break;
        case 4:
            filename += "layer4.jpg";
            break;
        case 5:
            filename += "layer5.jpg";
            break;
        case 6:
            filename += "layer6.jpg";
            break;
        case 7:
            filename += "layer7.jpg";
            break;
        default:
            break;
        }

        sp.showAndSaveSimularityMap(filename);
        cv::waitKey(3000);
    }

    delete network;
}

void test_TempFunc()
{
    // Transform 512*512 images to 32*32 images
    CztMod * cm = new CztMod();
    /*string s = "ABCDEFGHIJKLMNYZ";
    string t;
    for(int i=0; i<s.length(); ++i)
    {
        t = "";
        t.insert(t.begin(), s[i]);
        cm->resizeImage("/home/teaera/Work/RECORD/2013.7.22/512/"+t+".png", "/home/teaera/Work/RECORD/2013.7.22/32/"+t+".png", cv::Size(32, 32));
    }*/
}

void test_Quality()
{
    // To follow the habits from C, initialize i,j,k outside the loops first;
    int i, j, l;

    ImageSouceImpl isi;
    string imgs = "ABCDEFGHIJKLMNYZ", img;
    for(i=0; i<imgs.length(); ++i)
    {
        img = "";
        img.insert(img.begin(), imgs[i]);
        isi.addImage("/home/teaera/Work/RECORD/2013.7.22/32/" + img + ".png");
    }

    SupportedImageWidths siw = W32;
    uint nLayer = 4;
    //uint centroid_counts[]  = {96, 64, 32, 16};
    uint centroid_counts[]  = {64, 64, 32, 16};
    bool isUniform = true;
    DestinNetworkAlt * network = new DestinNetworkAlt(siw, nLayer, centroid_counts, isUniform);
    Destin * d = network->getNetwork();

    int frameCount = 1, maxCount = 1600;
    while(frameCount <= maxCount){
        frameCount++;
        if(frameCount % 10 == 0)
        {
            printf("Count %d;\n", frameCount);

            /*for(l=0; l<d->nLayers; ++l)
            {
                printf("Layer %d\n", l);
                for(i=0; i<d->nb[l]; ++i)
                {
                    i = network->getNode(l,0,0)->winner;
                    printf("winner: %d\n", i);

                    // Only display the 'ni' part!
                    for(j=0; j<network->getNode(l,0,0)->ni; ++j)
                    {
                        //printf("%f  ", d->uf_avgDelta[l][i*network->getNode(l,0,0)->ns+j]);
                        //printf("%f  ", d->uf_sigma[l][i*network->getNode(l,0,0)->ns+j]);
                        printf("%f  ", d->uf_absvar[l][i*network->getNode(l,0,0)->ns+j]);
                    }
                    printf("\n");

                    break;
                }
                printf("------\n");
            }
            printf("\n");*/

            /*l = nLayer-1;
            Node * currNode = network->getNode(l, 0, 0);
            uint currWinner = currNode->winner;
            printf("Centroid:\n");
            for(i=0; i<currNode->ni; ++i)
            {
                printf("%f  ", currNode->mu[currWinner*currNode->ns + i]);
            }
            printf("\n");
            printf("Observation:\n");
            Node ** nodes = currNode->children;
            for(i=0; i<4; ++i)
            {
                for(j=0; j<nodes[i]->nb; ++j)
                {
                    printf("%f  ", nodes[i]->beliefMal[j]);
                }
            }
            printf("\n");*/
        }

        isi.findNextImage();
        network->doDestin(isi.getGrayImageFloat());
    }

    uint tempLayer = 3;
    network->displayLayerCentroidImages(tempLayer, 1000);
    cv::waitKey(3000);
    network->saveLayerCentroidImages(tempLayer, "/home/teaera/Pictures/visualizer_layer7.jpg");

    delete network;
}

void test_SOM2()
{
    ImageSouceImpl isi;
    string imgs = "ABCDEFGHIJKLMNYZ", img;
    uint nImgs = imgs.length();
    for(int i=0; i<nImgs; ++i)
    {
        img = "";
        img.insert(img.begin(), imgs[i]);
        isi.addImage("/home/teaera/Work/RECORD/2013.7.22/32/" + img + ".png");
    }

    SupportedImageWidths siw = W32;
    uint nLayer = 4;
    //uint centroid_counts[]  = {64, 64, 32, 16};
    uint centroid_counts[]  = {4, 4, 4, 16};
    bool isUniform = true;
    DestinNetworkAlt * network = new DestinNetworkAlt(siw, nLayer, centroid_counts, isUniform);

    int frameCount = 1, maxCount = 16000;
    while(frameCount <= maxCount){
        frameCount++;
        if(frameCount % 10 == 0)
        {
            printf("Count %d;\n", frameCount);
        }

        isi.findNextImage();
        network->doDestin(isi.getGrayImageFloat());
    }

    for(int i=0; i<nLayer; ++i)
    {
        network->setLayerIsTraining(i, false);
    }

    // For testing
    for(int currLayer=0; currLayer<nLayer; ++currLayer)
    {
        Node * currNode = network->getNode(currLayer, 0, 0);
        uint nb = currNode->nb;
        uint dim = currNode->ni;
        printf("---Layer %d---\n", currLayer);
        printf("Centroids: %d; Dimensions:  %d;\n", nb, dim);
        uint som_rows = 200, som_cols = 200;
        uint som_train_iterations = 5000;

        // Different colors
        std::vector<float> hueVector;
        for(int i=0; i<nb; ++i)
        {
            hueVector.push_back((float)i / (float)nb);
        }

        ClusterSom som(som_rows, som_cols, dim);
        SomPresentor sp(som);

        // The centroids
        std::vector<std::vector<float> > muData;
        for(int i=0; i<nb; ++i)
        {
            std::vector<float> d;
            for(int j=0; j<dim; ++j)
            {
                d.push_back(currNode->mu[i*currNode->ns + j]);
            }
            muData.push_back(d);
            som.addTrainData(d.data());
        }
        printf("muData: %ld;\n", muData.size());

        // The observations
        std::vector<std::vector<float> > obsData;
        std::vector<int> obsWinner;
        int currSize = network->getNetwork()->layerSize[currLayer];
        for(int idxImg=0; idxImg<nImgs; ++idxImg)
        {
            isi.findNextImage();
            float * f = isi.getGrayImageFloat();
            // In case
            for(int i=0; i<nLayer*10; ++i)
            {
                network->doDestin(f);
            }

            int currWidth = (int)sqrt(currSize);
            for(int row=0; row<currWidth; ++row)
            {
                for(int col=0; col<currWidth; ++col)
                {
                    Node * tNode = network->getNode(currLayer, row, col);
                    std::vector<float> d;
                    if(currLayer == 0)
                    {
                        for(int i=0; i<tNode->ni; ++i)
                        {
                            d.push_back(f[tNode->inputOffsets[i]]);
                        }
                    }
                    else
                    {
                        Node ** childNodes = tNode->children;
                        for(int i=0; i<4; ++i)
                        {
                            for(int j=0; j<childNodes[i]->nb; ++j)
                            {
                                d.push_back(childNodes[i]->beliefMal[j]);
                            }
                        }
                    }
                    obsData.push_back(d);
                    som.addTrainData(d.data());

                    //
                    obsWinner.push_back(tNode->winner);
                }
            }

        }
        printf("obsData: %ld;\n", obsData.size());

        som.train(som_train_iterations);

        for(int i=0; i<muData.size(); ++i)
        {
            std::vector<float> d = muData.at(i);
            CvPoint cp = som.findBestMatchingUnit(d.data());
            sp.addSimMapMaker(cp.y, cp.x, hueVector[i], 10);
        }

        for(int i=0; i<obsData.size(); ++i)
        {
            std::vector<float> d = obsData.at(i);
            CvPoint cp = som.findBestMatchingUnit(d.data());
            sp.addSimMapMaker(cp.y, cp.x, hueVector[obsWinner[i]], 3);
        }

        string filename = "/home/teaera/Pictures/";
        switch(currLayer)
        {
        case 0:
            filename += "layer0.jpg";
            break;
        case 1:
            filename += "layer1.jpg";
            break;
        case 2:
            filename += "layer2.jpg";
            break;
        case 3:
            filename += "layer3.jpg";
            break;
        case 4:
            filename += "layer4.jpg";
            break;
        case 5:
            filename += "layer5.jpg";
            break;
        case 6:
            filename += "layer6.jpg";
            break;
        case 7:
            filename += "layer7.jpg";
            break;
        default:
            break;
        }

        sp.showAndSaveSimularityMap(filename);
        cv::waitKey(3000);
    }

    delete network;
}
