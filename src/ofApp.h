#pragma once

#include "ofMain.h"

#include <pwd.h>
#include <unistd.h>

#include "ofxAudioFile.h"
#include "ofxCropTexture.h"
#include "ofxCv.h"
#include "ofxOpenCv.h"
#include "ofxGui.h"
#include "ofxEasyFboGlitch.h"
#include "ofxTrueTypeFontUC.h"
#include "ofxSoundPlayerMultiOutput.h"
#include "ofxWarp.h"

#include "config.h"

class ofApp : public ofBaseApp{

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

    void audioOut(ofSoundBuffer & buffer);

    void initAppDataFolder();
    void motionDetection();
    void serialRead();
    void sleepCountdown();

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

    // ---------------------------------------------- VIDEO
    ofVideoPlayer               *mainVideo;
    ofImage                     *rgb;
    ofFbo                       *finalTexture;
    ofFbo                       *correctedFinalTexture;
    ofShader                    *colorCorrection;
    ofxWarpController           *warpManager;

    // ---------------------------------------------- AUDIO
    ofSoundStream               soundStream;

    float                       volume1, volume2, volume3, volume4;

    vector <float>              ch1Audio;
    vector <float>              ch2Audio;
    vector <float>              ch3Audio;
    vector <float>              ch4Audio;

    // ---------------------------------------------- GENERATIVE
    vector<string>              words;

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
    int                         _startImuHeading;
    int                         _imuHeading;
    float                       _motionFactor;

protected:
    bool                        preload;

};
