#pragma once

#include "ofMain.h"

#include <pwd.h>
#include <unistd.h>

#include "ofxCropTexture.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"
#include "ofxEasyFboGlitch.h"
#include "ofxPDSP.h"
#include "ofxTrueTypeFontUC.h"
#include "ofxXmlSettings.h"
#include "ofxWarp.h"

#include "config.h"

class ofApp : public ofBaseApp, pdsp::Wrapper{

public:
    void setup();
    void update();
    void draw();
    void drawGUI();
    void exit();

    void updateMovingWindow(ofEventArgs &e);
    void drawMovingWindow(ofEventArgs &e);
    void keyPressedMovingWindow(ofKeyEventArgs &e);
    void keyReleasedMovingWindow(ofKeyEventArgs &e);
    void mouseMovedMovingWindow(ofMouseEventArgs &e);
    void mouseDraggedMovingWindow(ofMouseEventArgs &e);
    void mousePressedMovingWindow(ofMouseEventArgs &e);
    void mouseReleasedMovingWindow(ofMouseEventArgs &e);

    void initAppDataFolder();
    void motionDetection();
    void serialRead();
    void sleepCountdown();

    void drawNumeros();
    void drawPalabras();
    void drawCuadrados();

    void playRandomCampana();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);

    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);

    shared_ptr<ofAppGLFWWindow> mainWindow;

    // ---------------------------------------------- 4TH Projection (indendent window)
    shared_ptr<ofAppGLFWWindow> movingWindow;

    ofFbo                       *movingFBO;
    ofFbo                       *glitchedFBO;
    ofFbo                       *finalMovingFbo;
    ofShader                    *desaturate;
    ofxEasyFboGlitch            *fboGlitch;
    ofRectangle                 movingBounds;
    ofxWarpController           *movingWarpManager;
    ofxTrueTypeFontUC           *font;
    bool                        isSecondaryWinLoaded;
    size_t                      waitTimeForFullscreen;
    bool                        isSecondaryFullscreen;
    float                       displacementX;
    int                         generativeState;
    int                         actualQuadrant;

    // ---------------------------------------------- VIDEO
    ofVideoPlayer               *mainVideo;
    ofImage                     *rgb;
    ofFbo                       *finalTexture;
    ofFbo                       *correctedFinalTexture;
    ofFbo                       *sectorL, *sectorC, *sectorR;
    ofShader                    *colorCorrection;
    ofxWarpController           *warpManager;

    // ---------------------------------------------- AUDIO
    pdsp::Engine                *engine;
    pdsp::PatchNode             mix;
    pdsp::Scope                 scope;
    std::mutex                  audioMutex;

    pdsp::ExternalInput         audioCH1;
    pdsp::ExternalInput         audioCH2;
    pdsp::ExternalInput         audioCH3;
    pdsp::ExternalInput         audioCH4;
    ofxAudioFile                audioCH1_file;
    ofxAudioFile                audioCH2_file;
    ofxAudioFile                audioCH3_file;
    ofxAudioFile                audioCH4_file;
    ofSoundBuffer               audioCH1_buffer;
    ofSoundBuffer               audioCH2_buffer;
    ofSoundBuffer               audioCH3_buffer;
    ofSoundBuffer               audioCH4_buffer;

    double                      main_playhead;
    double                      main_step;


    pdsp::ExternalInput         campana;
    ofxAudioFile                campanaFile;
    ofSoundBuffer               campana_buffer;
    ofSoundBuffer               emptyBuffer;
    double                      campana_playhead;
    double                      campana_step;

    int                         randomCHCamapana;
    bool                        playCampana;

    // ---------------------------------------------- GENERATIVE
    vector<string>              words;
    size_t                      resetWordTime;
    int                         waitForWord;
    int                         wordCounter;

    // ---------------------------------------------- GUI
    bool                        drawGui;

    // ---------------------------------------------- Motion Detection Vars
    ofVideoGrabber              *vidGrabber;
    ofxCvColorImage             *colorImg;
    ofxCvGrayscaleImage         *grayPrev;
    ofxCvGrayscaleImage         *grayNow;
    ofxCvGrayscaleImage         *motionImg;
    unsigned char               *blackPixels;
    int                         motion_threshold;
    int                         motion_noiseCompensation;
    int                         _totPixels;
    int                         frameCounter;
    int                         numPixelsChanged;

    size_t                      resetTime;
    size_t                      waitTime;
    bool                        isSystemSleeping;

    // ---------------------------------------------- IMU from serial port (arduino)
    ofSerial                    serial;
    unsigned char               bytesReturned[1];
    string                      message;
    string                      messageBuffer;
    bool                        serialAttached;
    bool                        firstValueRead;
    size_t                      readingCounter;

private:
    void audioProcess(float *input, int bufferSize, int nChannels);

    int                         _startImuHeading;
    int                         _imuHeading;
    float                       _motionFactor;

protected:
    ofxXmlSettings              xmlSettings;
    bool                        settingsLoaded;
    int                         _audio_device;
    int                         _sample_rate;
    int                         _buffer_size;
    int                         _output_channels;
    int                         _input_channels;
    float                       ch1_vol;
    float                       ch2_vol;
    float                       ch3_vol;
    float                       ch4_vol;
    float                       campana_vol;
    float                       voz_vol;
    string                      _serial_device;
    int                         _start_imu_angle;
    int                         _end_imu_angle;
    float                       _motion_threshold;
    int                         _wait_for_sleep_min;
    float                       _glitch_bleach;
    float                       _glitch_desaturation;

    bool                        preload;

};

//--------------------------------------------------------------
static inline float hardClip(float x){
    float x1 = fabsf(x + 1.0f);
    float x2 = fabsf(x - 1.0f);

    return 0.5f * (x1 - x2);
}
