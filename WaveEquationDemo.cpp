#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <math.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <random> //for random engine
#include <CL/cl.hpp>

using namespace std;

default_random_engine generator;
normal_distribution<float> dist1(0,0.2);
uniform_real_distribution<float> dist2(-1.0,1.0);

struct particle {
    Vector3 position;
    Vector3 velocity;
    float radius;
    float mass;
    Color color;
};

Color Colorizer(int binval) {
    Color c;
    binval*=4;
    c.a=255;
    c.r=(binval<255)?binval:255;
    c.g=binval/10;
    c.b=0;
return c;
}


int main()
{
    bool render_freeze=false;
    bool render_histogram=false;
    bool fancy_histogram=false;
    bool set_fancy_cam=false; //toggles for whether we need to update camera perspective
    bool set_regular_cam=false; //toggles for whether we need to update camera perspective
    int WinXsize=1200, WinYsize=900;
    int MAXPARTS=5000;
    float amplitude=400.0;
    int number_of_bins=201;
    float bin_width=2.0*amplitude/(number_of_bins-1);
    int looper=0;

    int THREE_D_FPS=60;
    int NORMAL_FPS=30;

    //Initialize Raylib
    InitWindow(WinXsize, WinYsize, "Wave-Particle Demo");
    SetWindowPosition(500,50);
    Camera3D camera;
    camera.position={0.0,0.0,-600.0};
    camera.target={0.0,0.0,0.0};
    camera.up={0.0,1.0,0.0};
    camera.fovy=60.0;
    camera.projection=CAMERA_PERSPECTIVE;
    UpdateCamera(&camera);
    Camera oldcamera=camera;

    SetTargetFPS(NORMAL_FPS);

//Init OpenCL
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);
    if(all_platforms.size()==0){
        std::cout<<" No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    cl::Platform default_platform=all_platforms[0];
    std::cout << "Using platform: "<<default_platform.getInfo<CL_PLATFORM_NAME>()<<"\n";

    //get default device of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if(all_devices.size()==0){
        std::cout<<" No devices found. Check OpenCL installation!\n";
        exit(1);
    }
    cl::Device default_device=all_devices[0];
    std::cout<< "Using device: "<<default_device.getInfo<CL_DEVICE_NAME>()<<"\n";

    cl::Context context({default_device});

    //Define an output buffer for data
    cl::Buffer buffer_VEL(context,CL_MEM_READ_WRITE,sizeof(float)*MAXPARTS);
    cl::Buffer buffer_POS(context,CL_MEM_READ_WRITE,sizeof(float)*MAXPARTS);
    cl::Buffer buffer_OFFSET(context,CL_MEM_READ_WRITE,sizeof(float)*MAXPARTS);
    cl::Buffer buffer_MASS(context,CL_MEM_READ_WRITE,sizeof(float)*MAXPARTS);
    cl::Buffer buffer_FORCE(context,CL_MEM_READ_WRITE,sizeof(float)*1201);  // TODO: Update to general

    //Load kernel from file
    ifstream t("./kernel.cl");
    if (!t) { cout << "Error Opening Kernel Source file\n"; exit(-1); }
    std::string kSrc((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
    cl::Program::Sources sources(1, make_pair(kSrc.c_str(), kSrc.length()));

    cl::Program program(context,sources);
    if(program.build({default_device})!=CL_SUCCESS){
        std::cout<<" Error building: "<<program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device)<<"\n";
        exit(1);
    }

    //create queue to which we will push commands for the device.
    cl::CommandQueue myqueue(context,default_device);

    cl::Kernel mykernel=cl::Kernel(program,"sinusoidal", NULL);

//Init variables
    Vector3 cpos={0.0,0.0,0.0};
    Vector3 cvel={0.0,0.0,0.0};
    float time=0.0; //12500.0;
    float timestep=0.01;
    int screennum=0;
    char timestamp[15];
    char page_number[5];
    char page_time_in[15];
    char page_time_out[15];

    float velocities[MAXPARTS]; //for buffers.  Will be better eventually to define a struct in kernel and push the whole particle struct to it
    float positions[MAXPARTS];
    float masses[MAXPARTS];
    vector<int> bins;
    bins.resize(number_of_bins);
    vector<particle> particles;
    vector< vector<int> > history;


    particles.reserve(MAXPARTS);
    particle newpart;

    //Modify initial distribution of particle positions and velocities here
    for (uint i=0;i<MAXPARTS;i++) {
        newpart.radius=1;
        newpart.mass=1;
        newpart.color=(Color){255,255,255,255};
        newpart.position=(Vector3){cpos.x,0.05f*(float)i,0};
        newpart.velocity=cvel;
        newpart.velocity.x=0.5;
        newpart.position.x+=32.0*dist1(generator);
        //newpart.velocity.x+=dist1(generator); //gaussian distribution
        newpart.velocity.x+=1.0*floor(4.0*dist2(generator)); //quantized velocities of uniform distribution
        //newpart.velocity.x+=8.0*dist2(generator); // uniform distribution, not quantized
        //newpart.velocity.x+=0.1*floor(200*dist2(generator)); // quantized velocities of gaussian distribution
        velocities[i]=newpart.velocity.x; //Load velocity upon creation
        positions[i]=newpart.position.x; //Load positions just so buffer isn't filled with uninitalized data
        masses[i]=newpart.mass; //Load masses
        particles.push_back(newpart);
    }

    cpos.y=-5.0;



//For sinusoidal kernel
//Load velocities once
    myqueue.enqueueWriteBuffer(buffer_VEL,CL_TRUE,0,sizeof(float)*particles.size(),&velocities[0],NULL,NULL);
    myqueue.enqueueWriteBuffer(buffer_OFFSET,CL_TRUE,0,sizeof(float)*particles.size(),&positions[0],NULL,NULL);
    mykernel.setArg(0,sizeof(float),&time);
    mykernel.setArg(1,sizeof(float),&amplitude);
    mykernel.setArg(2,buffer_VEL);
    mykernel.setArg(3,buffer_POS);
    mykernel.setArg(4,buffer_OFFSET); //Added an offset to starting position which is loaded from initial particle position

/*
//For general kernel
    myqueue.enqueueWriteBuffer(buffer_VEL,CL_TRUE,0,sizeof(float)*particles.size(),&velocities[0],NULL,NULL);
    myqueue.enqueueWriteBuffer(buffer_VEL,CL_TRUE,0,sizeof(float)*particles.size(),&positions[0],NULL,NULL);
    myqueue.enqueueWriteBuffer(buffer_VEL,CL_TRUE,0,sizeof(float)*particles.size(),&masses[0],NULL,NULL);  //Mass only needs to be loaded once
    mykernel.setArg(0,sizeof(float),&timestep);
    mykernel.setArg(1,buffer_VEL);
    mykernel.setArg(2,buffer_POS);
    mykernel.setArg(3,buffer_MASS);
*/

    char answer;
    cout<<"Enter a letter to continue"<<endl;
    cin>>answer;


//The render loop
    while (!WindowShouldClose()) {

//Update

        if (IsKeyPressed(KEY_F)) {render_freeze=true;}
        if (IsKeyPressed(KEY_G)) {render_freeze=false;}
        if (IsKeyPressed(KEY_H)) {render_histogram=true; fancy_histogram=false; render_freeze=true;}
        if (IsKeyPressed(KEY_J)) {render_histogram=false; fancy_histogram=false; render_freeze=false; set_regular_cam=true;}
        if (IsKeyPressed(KEY_N)) {render_histogram=true; fancy_histogram=true; render_freeze=true; set_fancy_cam=true;}
        if (IsKeyPressed(KEY_EQUAL)) {NORMAL_FPS+=10; SetTargetFPS(NORMAL_FPS);}
        if (IsKeyPressed(KEY_MINUS)) {NORMAL_FPS-=10; if (NORMAL_FPS<0) NORMAL_FPS=10; SetTargetFPS(NORMAL_FPS);}
        if (IsKeyPressed(KEY_ZERO)) {NORMAL_FPS=0; SetTargetFPS(NORMAL_FPS);}


    if (!render_freeze) {
        time=time+timestep;

        //cpos.x=amplitude*sin(cvel.x*time); //Update central particle on its own
        cvel.x=velocities[0];
        cpos.x=positions[0];
        //cout<<"Loc: "<<loc<<" Vel:"<<cvel.x<<" Pos:"<<cpos.x<<endl;


        //For sinusoidal kernel
        mykernel.setArg(0,sizeof(float),&time); //Only need to update time for each kernel call
        myqueue.enqueueNDRangeKernel(mykernel,cl::NullRange,cl::NDRange(particles.size()),cl::NullRange);

        //Get data back out
        myqueue.enqueueReadBuffer(buffer_POS,CL_TRUE,0,sizeof(float)*particles.size(),&positions[0], NULL, NULL);

        for (uint i=0;i<particles.size();i++) {
                particles[i].position.x=positions[i];
        }

        /*
        //For general kernel
        //Load velocity and position values
        for (uint i=0;i<MAXPARTS;i++) {
            velocities[i]=particles[i].velocity.x;
            positions[i]=particles[i].position.x;
        }
        myqueue.enqueueWriteBuffer(buffer_VEL,CL_TRUE,0,sizeof(float)*particles.size(),&velocities[0],NULL,NULL);
        myqueue.enqueueWriteBuffer(buffer_POS,CL_TRUE,0,sizeof(float)*particles.size(),&positions[0],NULL,NULL);

        mykernel.setArg(1,buffer_VEL);
        mykernel.setArg(2,buffer_POS);
        myqueue.enqueueNDRangeKernel(mykernel,cl::NullRange,cl::NDRange(particles.size()),cl::NullRange);

        //Get updated velocity and position values back
        myqueue.enqueueReadBuffer(buffer_VEL,CL_TRUE,0,sizeof(float)*particles.size(),&velocities[0], NULL, NULL);
        myqueue.enqueueReadBuffer(buffer_POS,CL_TRUE,0,sizeof(float)*particles.size(),&positions[0], NULL, NULL);

        for (uint i=0;i<particles.size();i++) {
            particles[i].velocity.x=velocities[i];
            particles[i].position.x=positions[i];
        }
        //Done
        */
    }

//Draw
    //cout<<"Camera at: x="<<camera.position.x<<" y="<<camera.position.y<<" z="<<camera.position.z<<endl;
    //cout<<"Target at: x="<<camera.target.x<<" y="<<camera.target.y<<" z="<<camera.target.z<<endl;

    if (render_histogram==false) {
            BeginDrawing();
            if (set_regular_cam) {
                //cout<<"Resetting Camera"<<endl;
                camera=oldcamera;
                UpdateCamera(&camera);
                SetTargetFPS(NORMAL_FPS);
                set_regular_cam=false;
            }

            BeginMode3D(camera);
            ClearBackground(BLACK);

            DrawSphere(cpos,1.0,RED);
            for (uint i=0, idx=0;i<MAXPARTS;i++) {
                DrawCube(particles[i].position,1.0,1.0,1.0,WHITE);
                //Tired of out-of-bounds segfaults here.  Kludging with a clamp.
                idx=(uint)floor((particles[i].position.x+amplitude)/bin_width);
                if (idx<0) idx=0;
                if (idx>number_of_bins) idx=number_of_bins;
                bins[idx]++; //fill bins while we draw
            }

            //Draw histogram
            for (int i=0;i<number_of_bins-1;i++){
                DrawCube((Vector3){-amplitude+i*bin_width,-300+(bins[i]/4),1},bin_width,bins[i]/2,bin_width,BLUE);
            }

            EndMode3D();

            if (render_freeze==false) { // Don't store history when render_freeze is true, or else we just streak the same values
                history.push_back(bins);
            }

            for (int i=0;i<number_of_bins;i++){
                bins[i]=0;
            }

            DrawFPS(10,10);
            sprintf(timestamp,"%12.3f",time);
            DrawText(timestamp,1000,10,20,GREEN);
            EndDrawing();

        //End Live Mode Draw Routine
    }
    else if (!fancy_histogram) {
        //Begin 2D History Draw Routine

            int plotstart = 0;
            int plotend = 0;
            int spacer=0;
            int maxwidth = number_of_bins*floor(1200/number_of_bins)+1;
            int LinesPerScreen = floor(1200/number_of_bins)*900;

            if (IsKeyPressed(KEY_W)) {
                screennum--;
                screennum=(screennum<0)?0:screennum;
            }
            if (IsKeyPressed(KEY_S)) {
                screennum++;
                if (screennum*LinesPerScreen>history.size()) {cout<<"At Last Screen"<<endl; screennum--;}
            }

            plotstart=screennum*LinesPerScreen;
            plotend=plotstart+LinesPerScreen;
            plotend=(plotend>history.size())?history.size():plotend;

            //cout<<"maxwdith="<<maxwidth<<" Lines Per Screen="<<LinesPerScreen<<" History size="<<history.size()<<" Plot start="<<plotstart<<" Plot end="<<plotend<<endl;

            BeginDrawing();
            ClearBackground(BLACK);

            Vector3 plotspot;
            for (int i=plotstart;i<plotend;i++) {
                for (int j=0;j<history[i].size();j++) {
                    plotspot.x=j+number_of_bins*((i-plotstart)/900);
                    spacer=10*(((i-plotstart)/900)+1);
                    while (plotspot.x>maxwidth) {plotspot.x-=maxwidth;}
                    plotspot.y=((i-plotstart)%900);
                    plotspot.z=0;
                    DrawPixel(plotspot.x+spacer,plotspot.y,Colorizer(history[i][j]));
                }
            }
            DrawLine(plotspot.x-number_of_bins+spacer,plotspot.y+1,plotspot.x+spacer,plotspot.y+1,GREEN);
            DrawFPS(10,10);
            sprintf(page_number,"%i",screennum);
            sprintf(page_time_in,"%12.3f",0.01*plotstart);
            sprintf(page_time_out,"%12.3f",0.01*plotend);
            DrawText("Page:",1100,10,20,GREEN);
            DrawText(page_number,1170,10,20,GREEN);
            DrawText("From Time:",1080,50,20,GREEN);
            DrawText(page_time_in,1050,70,20,GREEN);
            DrawText("To Time:",1100,110,20,GREEN);
            DrawText(page_time_out,1050,130,20,GREEN);

            EndDrawing();
    }
    else {
        //Begin 3D History Draw Routine
        if (set_fancy_cam) {
            SetTargetFPS(THREE_D_FPS);
            camera.position={-100.0,220.0,-160.0};
            camera.target={-100.0,86.0,10.0};
            camera.up={0.0,1.0,0.0};
            UpdateCamera(&camera);
            set_fancy_cam=false;
        }

        BeginDrawing();
        BeginMode3D(camera);
        ClearBackground(BLACK);

        float height=0.0;
        Vector3 pos=(Vector3){0.0,0.0,0.0};

        //DrawGizmo((Vector3){0.0,0.0,0.0});
            for (int z=looper+0;z<looper+100;z++) {
            //cout<<"Row z="<<z<<endl;
                for (int x=0;x<number_of_bins;x++) {
                    pos.x=-300+2*x;
                    pos.z=2*(z-looper);
                    height=history[z][x]/4.0;
                    //DrawCube(pos,2.0,height,2.0,BLUE);  //Both Cube and Wires gives best visualization but slow performance
                    DrawCubeWires(pos,2.0,height,2.0,Colorizer(history[z][x]));
                    if (height>0.1) {
                        DrawCube(pos,2.0,height,2.0,Colorizer(history[z][x]));
                        //DrawCubeWires(pos,2.0,height,2.0,BLACK); //(Color){0,192,255,255}
                    }
                    else {
                        DrawCube(pos,2.0,height,2.0,BLACK);
                    }
                    //cout<<"pos.x="<<pos.x<<" pos.y="<<pos.y<<" pos.z="<<pos.z<<" height="<<height<<endl;
                }
            }
        looper++;
        looper=(looper>history.size()-101)?0:looper;

        EndMode3D();
        DrawFPS(10,10);
        EndDrawing();

    }

    } //End render loop

    return 0;
}
