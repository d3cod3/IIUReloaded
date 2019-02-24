#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // ---------------------------------------------- OF Stuff
    ofSetFrameRate(FPS);
    ofEnableAntiAliasing();

    // ---------------------------------------------- MAIN Projection (triple head)
    mainWindow = dynamic_pointer_cast<ofAppGLFWWindow>(ofGetCurrentWindow());
    mainWindow->setWindowTitle("Main Projection (Triple Head)");
    mainWindow->setVerticalSync(true);

    // ---------------------------------------------- 4TH Projection (indendent window)
    ofGLFWWindowSettings settings;
    settings.setGLVersion(2,1);
    settings.shareContextWith = mainWindow;
    settings.decorated = true;
    settings.resizable = true;
    settings.stencilBits = 0;
    settings.setPosition(ofDefaultVec2(0,0));
    settings.setSize(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);

    movingWindow = dynamic_pointer_cast<ofAppGLFWWindow>(ofCreateWindow(settings));
    movingWindow->setWindowTitle("Moving Projector");
    movingWindow->setVerticalSync(true);
    movingWindow->setWindowPosition(MAIN_SCREEN_W,0);

    /*ofAddListener(movingWindow->events().draw,this,&ofApp::drawMovingWindow);
    ofAddListener(movingWindow->events().keyPressed,this,&ofApp::keyPressedMovingWindow);
    ofAddListener(movingWindow->events().keyReleased,this,&ofApp::keyReleasedMovingWindow);
    ofAddListener(movingWindow->events().mouseMoved ,this,&ofApp::mouseMovedMovingWindow);
    ofAddListener(movingWindow->events().mouseDragged ,this,&ofApp::mouseDraggedMovingWindow);
    ofAddListener(movingWindow->events().mousePressed ,this,&ofApp::mousePressedMovingWindow);
    ofAddListener(movingWindow->events().mouseReleased ,this,&ofApp::mouseReleasedMovingWindow);*/

    // ---------------------------------------------- VIDEO

    // ---------------------------------------------- AUDIO

    // ---------------------------------------------- GUI
    drawGui             = false;

    // ---------------------------------------------- Motion Detection Vars
    vidGrabber = new ofVideoGrabber();
    vidGrabber->setDeviceID(0);
    vidGrabber->setDesiredFrameRate(FPS);
    vidGrabber->initGrabber(WEBCAM_W,WEBCAM_H);
    _totPixels          = WEBCAM_W*WEBCAM_H;
    frameCounter        = 0;
    numPixelsChanged    = 0;
    _motionFactor       = 0.0f;

    colorImg    = new ofxCvColorImage();
    grayPrev    = new ofxCvGrayscaleImage();
    grayNow     = new ofxCvGrayscaleImage();
    motionImg   = new ofxCvGrayscaleImage();

    colorImg->allocate(WEBCAM_W,WEBCAM_H);
    grayPrev->allocate(WEBCAM_W,WEBCAM_H);
    grayNow->allocate(WEBCAM_W,WEBCAM_H);
    motionImg->allocate(WEBCAM_W,WEBCAM_H);

    blackPixels = new unsigned char[_totPixels];
    for(unsigned int b=0;b<_totPixels;b++){
        blackPixels[b] = 0;
    }

    motion_threshold            = 100;
    motion_noiseCompensation    = 1000;

    resetTime                   = ofGetElapsedTimeMillis();
    waitTime                    = WAIT_FOR_SLEEP_MS;
    isSystemSleeping            = false;

    // ---------------------------------------------- IMU from serial port (arduino)
    serialAttached = false;
    //serial.listDevices();
    if(serial.setup("/dev/ttyACM0", 9600)){
        serialAttached = true;
    }

    message = "";
    messageBuffer = "";
    _imuHeading                 = 0;


    // ----------------------------------------------
    // let something load in loop (GL stuff)
    preload = false; // last setup line
}

//--------------------------------------------------------------
void ofApp::update(){

    // --------------------------------------------------- Motion Detection (webcam as activation sensor)
    motionDetection();

    // --------------------------------------------------- Serial Read (data from arduino)
    serialRead();

    // --------------------------------------------------- System sleep mode control
    sleepCountdown();

}

//--------------------------------------------------------------
void ofApp::draw(){
    //-------------------------------------
    if(!preload){
        preload = true;
        //mainWindow->toggleFullscreen();
    }
    //-------------------------------------

    // background
    ofBackground(0);
    // GUI
    drawGUI();
    // RELOAD
    if(isSystemSleeping){

    }else{

    }

}

//--------------------------------------------------------------
void ofApp::drawGUI(){
    if(drawGui){
        ofSetColor(255);
        ofDrawBitmapString("Motion Factor: " + ofToString(_motionFactor),20,20);
        ofDrawBitmapString("4th projector Heading: " + ofToString(_imuHeading),20,40);
        int remainingTime = static_cast<int>((waitTime-(ofGetElapsedTimeMillis()-resetTime))/1000);
        ofDrawBitmapString("Sleeping mode in: "+ofToString(remainingTime)+" seconds",20,60);
    }
}

//--------------------------------------------------------------
void ofApp::exit(){

}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){

}

//--------------------------------------------------------------
void ofApp::motionDetection(){
    vidGrabber->update();

    if(vidGrabber->isFrameNew()){
        colorImg->setFromPixels(vidGrabber->getPixels());
        colorImg->updateTexture();

        if(frameCounter > 5){
            *grayNow = *colorImg;
            grayNow->updateTexture();

            motionImg->absDiff(*grayPrev, *grayNow);
            motionImg->updateTexture();
            cvThreshold(motionImg->getCvImage(), motionImg->getCvImage(), motion_threshold, 255, CV_THRESH_TOZERO); // anything below threshold, drop to zero (compensate for noise)
            numPixelsChanged = motionImg->countNonZeroInRegion(0, 0, WEBCAM_W, WEBCAM_H);

            if(numPixelsChanged >= motion_noiseCompensation){ // noise compensation
                *grayPrev = *grayNow; // save current frame for next loop
                cvThreshold(motionImg->getCvImage(), motionImg->getCvImage(), motion_threshold, 255, CV_THRESH_TOZERO);// chop dark areas
                motionImg->updateTexture();
                _motionFactor = ofClamp(static_cast<float>(numPixelsChanged)/static_cast<float>(_totPixels),0.0f,1.0f);
            }else{
                motionImg->setFromPixels(blackPixels,WEBCAM_W, WEBCAM_H);
                motionImg->updateTexture();
                _motionFactor = 0.0f;
            }

            // check for system state (on/sleep)
            if(_motionFactor > SLEEP_THRESHOLD){
                // we have movement, reset the sleep countdown
                resetTime = ofGetElapsedTimeMillis();
                isSystemSleeping = false;
            }

        }
    }

    frameCounter++;
}

//--------------------------------------------------------------
void ofApp::serialRead(){
    if (serialAttached && serial.available() > 0){
        while (serial.available() > 0){
            serial.readBytes(bytesReturned, 1);

            if (*bytesReturned == '\n'){
                message = messageBuffer;
                messageBuffer = "";
                _imuHeading = ofToInt(message);
            }else{
                messageBuffer += *bytesReturned;
            }
        }
        // clear the bytes buffer
        memset(bytesReturned, 0, 1);
    }
}

//--------------------------------------------------------------
void ofApp::sleepCountdown(){
    if(ofGetElapsedTimeMillis()-resetTime > waitTime && !isSystemSleeping){
        isSystemSleeping = true;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == 'f'){
        mainWindow->toggleFullscreen();
    }else if(key == 'g'){
        drawGui = !drawGui;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
