//g++ cameraInput.cpp `pkg-config opencv --cflags --libs`
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include<iostream>
#include<string>
#include<vector>
#include<stdio.h>

using namespace cv;
using namespace std;


string winName="input";
int X=300, Y=200, GAP=60, SIZE=20;
Scalar m[3][3]={Scalar(200)};
char clrs[3][3]={'\0'};
const char outFileName[]="cube-color.txt", configFileName[]="color-config.txt";
double prc=10;
//vxy recPos={{}};

class color{
    public:
    char clr;
    double min[3], max[3]; //min GBR and Max GBR
    color(){
        for(int i=0;i<3;i++){
            min[i]=256;
            max[i]=-1;
        }
    }
};

color colors[6];

char getClr(Scalar &s){
    int i,j;
    for(i=0;i<6;i++){
        for(j=0;j<3;j++){
            if(s[j]<colors[i].min[j]-prc || s[j]>colors[i].max[j]+prc ){
                break;
            }
        }
        if(j>=3){
            break;
        }
    }
    if(i>=6){
        cout<<"[ERROR] Can not determine ["<<s[0]<<", "<<s[1]<<", "<<s[2]<<"] this color. Please rescan or reconfigure.\n";
        return 'u';
    }
    return colors[i].clr;
}

bool readColors(Mat& frame){ 
    bool success=true;
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            int x=X+(i*GAP), y=Y+(j*GAP);
            Mat mask= Mat::zeros(frame.rows, frame.cols, CV_8U);
            mask(Rect(x,y, SIZE, SIZE))=1;
            m[i][j]=mean(frame, mask);   
            char ch=getClr(m[i][j]);
            if(ch=='u'){
                success=false;
            }
            clrs[i][j]=ch;
        }
    }
    return success;
}

void adjustFrame(Mat &frame){
    Mat hsv;
    cvtColor(frame, hsv, CV_BGR2HSV);
    const unsigned char hue_shift = 0, sat_shift=10;

    for (int j = 0; j < frame.rows; j++){
        for (int i = 0; i < frame.cols; i++){
            signed short h = hsv.at<Vec3b>(j, i)[0];
            signed short s = hsv.at<Vec3b>(j, i)[1];
            signed short h_plus_shift = h;
            h_plus_shift += hue_shift;
            if (h_plus_shift < 0)
                h = 180 + h_plus_shift;
            else if (h_plus_shift > 180)
                h = h_plus_shift - 180;
            else
                h = h_plus_shift;
            hsv.at<Vec3b>(j, i)[0] = static_cast<unsigned char>(h);
            hsv.at<Vec3b>(j, i)[1]+=sat_shift;
        }
    }
    cvtColor(hsv, frame, CV_HSV2BGR);
    imshow(winName, frame);
}

bool configureColors(){
    FILE *cfgfp=fopen(configFileName, "w");
    if(cfgfp==NULL){
        cout<<"[ERROR] Color configuration file could not be created!\n";
        return false;
    }

    VideoCapture cap(0);
    Mat frame, frame2;
    if(!cap.isOpened()){
        cout<<"[ERROR] Camera could not be opened!\n";
        return false;
    }
    cout<<"Camera opened.\n";
    string clrs="rbgywo";
    int c=0;
    int x=500, y=300;
    bool read=false;
    cout<<"Bring "<<clrs[c++]<<" color..\n";
    Scalar scl=Scalar(255);
    for(;;){
        cap>>frame;
        namedWindow("configure", WINDOW_NORMAL);
        resizeWindow("configure", 1600, 860);
        flip(frame, frame, 1);
        frame.copyTo(frame2);
        rectangle(frame2, Rect(x, y, SIZE, SIZE), scl, 3);
        imshow("configure", frame2);
        int key=waitKey(20);
        if(key==27){
            //ESC
            cout<<"Camera closed.\n";
            return false;
        }else if(key==32){
            //SPACE
            Mat mask= Mat::zeros(frame.rows, frame.cols, CV_8U);
            mask(Rect(x,y, SIZE, SIZE))=1;
            scl=mean(frame, mask);
            for(int i=0;i<3;i++){
                if(scl[i]<colors[c-1].min[i]){
                    colors[c-1].min[i]=scl[i];
                }
                if(scl[i]>colors[c-1].max[i]){
                    colors[c-1].max[i]=scl[i];
                }
            }
            read=true;
        }else if(key==10){
            //ENTER
            if(read){
                read=false;
                fprintf(cfgfp, "%c ", clrs[c-1]);
                for(int j=0;j<3;j++){
                    fprintf(cfgfp, "%lf %lf ", colors[c-1].min[j], colors[c-1].max[j]);
                }
                fprintf(cfgfp, "\n");
                if(c==6){
                    fclose(cfgfp);
                    cap.release();
                    destroyWindow("configure");
                    return true;
                }
                cout<<"Bring "<<clrs[c++]<<" color..\n";
                
            }else{
                cout<<"Read color first using SPACE\n";
            }
        }
    }
}

const int sliderVal_max = 100;
int sliderVal;

static void on_trackbar( int, void* )
{
   prc=sliderVal;
}


bool readCamera(){
    string str[7]={"front", "right", "back", "left", "up", "down", ""};
    int s=0;
    while(true){
        cout<<"Do you wabt to configure colors first?\n[y/n]: ";
        char ch=getchar();
        if(ch=='y' || ch=='Y'){
            if(!configureColors()){
                return false;
            }
            cout<<"\nConfigured colors successfully!\n";
            break;
        }else if(ch=='n'||ch=='N'){
            break;
        }
    }
    FILE *cfgfp=fopen(configFileName, "r");
    if(cfgfp==NULL){
        cout<<"[ERROR] Color configuration file could not be opened!\n";
        return false;
    }
    //read color config
    cout<<"Reading color configuration..\n";
    for(int i=0;i<6;i++){
        fscanf(cfgfp, "%c ", &colors[i].clr);
        cout<<colors[i].clr<<" ";
        for(int j=0;j<3;j++){
            fscanf(cfgfp, "%lf %lf ", &colors[i].min[j], &colors[i].max[j]);
        }
    }
    cout<<"..done!\n";

    FILE *outfp=fopen(outFileName, "w");
    bool read=false;
    if(outfp==NULL){
        cout<<"[ERROR] Output file could not be created!\n";
        return false;
    }
    VideoCapture cap(0);
    Mat frame, frame2;
    if(!cap.isOpened()){
        cout<<"[ERROR] Camera could not be opened!\n";
        return false;
    }
    cout<<"Camera opened.\n";
    cout<<"Bring "<<str[s++]<<" side for input..\n";
    for(;;){
        cap>>frame;
        namedWindow(winName, WINDOW_NORMAL);
        resizeWindow(winName, 1600, 860);
        //create taskbar
        char TrackbarName[50]="Adjust Precession";
        createTrackbar( TrackbarName, winName, &sliderVal, sliderVal_max, on_trackbar );
        
        flip(frame, frame, 1);
        frame.copyTo(frame2);
        for(int i=0;i<3;i++){
            for(int j=0;j<3;j++){
                //boundingRect
                rectangle(frame2, Rect(X+(i*GAP), Y+(j*GAP), SIZE, SIZE), m[i][j], 3);
            }
        }
        //adjustFrame(frame);
        imshow(winName, frame2);
        int key=waitKey(20);
        if(key==27){
            //ESC
            cout<<"Camera closed.\n";
            return false;
        }else if(key==32){
            //SPACE
            bool success=readColors(frame);
            for(int i=0;i<3;i++){
                for(int j=2; j>=0;j--){
                    printf("%c ", clrs[j][i]);
                }
                printf("\n");
            }
            if(!success){
                cout<<"[ERROR] Invalid read\n";
                read=false;
            }else{
                cout<<"Read success..\n";
                read=true;
            }
            
        }else if(key==10){
            //ENTER
            if(read){
                read=false;
                cout<<"Final input..\n";
                for(int i=0;i<3;i++){
                    for(int j=2; j>=0;j--){
                        fprintf(outfp, "%c ", clrs[j][i]);
                        printf("%c ", clrs[j][i]);
                    }
                    fprintf(outfp, "\n");
                    printf("\n");
                }
                fprintf(outfp, "\n");
                if(s==6){
                    fclose(outfp);
                    return true;
                }
                cout<<"Bring "<<str[s++]<<" side for input..\n";
                
            }else{
                cout<<"Read color first using SPACE\n";
            }

        }
        
    }
}

// int main(){
//     if(readCamera()){
//         cout<<"Successfully read cube from camera.\n";
//     }else{
//         cout<<"[ERROR] Camera input error\n";
//         exit(1);
//     }
// }