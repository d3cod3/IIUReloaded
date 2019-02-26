#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // ---------------------------------------------- OF Stuff
    ofSetFrameRate(FPS);
    ofEnableAntiAliasing();
    initAppDataFolder();

    // ---------------------------------------------- MAIN Projection (triple head)
    mainWindow = dynamic_pointer_cast<ofAppGLFWWindow>(ofGetCurrentWindow());
    mainWindow->setWindowTitle("Main Projection (Triple Head)");
    mainWindow->setVerticalSync(true);
    mainWindow->setWindowShape(MAIN_SCREEN_W,MAIN_SCREEN_H);

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

    ofAddListener(movingWindow->events().update,this,&ofApp::updateMovingWindow);
    ofAddListener(movingWindow->events().draw,this,&ofApp::drawMovingWindow);
    ofAddListener(movingWindow->events().keyPressed,this,&ofApp::keyPressedMovingWindow);
    ofAddListener(movingWindow->events().keyReleased,this,&ofApp::keyReleasedMovingWindow);
    ofAddListener(movingWindow->events().mouseMoved ,this,&ofApp::mouseMovedMovingWindow);
    ofAddListener(movingWindow->events().mouseDragged ,this,&ofApp::mouseDraggedMovingWindow);
    ofAddListener(movingWindow->events().mousePressed ,this,&ofApp::mousePressedMovingWindow);
    ofAddListener(movingWindow->events().mouseReleased ,this,&ofApp::mouseReleasedMovingWindow);

    // SETUP 4TH projection
    movingFBO           = new ofFbo();
    glitchedFBO         = new ofFbo();
    finalMovingFbo      = new ofFbo();
    desaturate          = new ofShader();
    fboGlitch           = new ofxEasyFboGlitch();
    movingWarpManager   = new ofxWarpController();
    font                = new ofxTrueTypeFontUC();

    movingFBO->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H,GL_RGBA,2);
    glitchedFBO->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H,GL_RGBA,2);
    finalMovingFbo->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H,GL_RGBA,2);
    fboGlitch->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    desaturate->load("videos/movingProjectionShader");
    font->load("videos/IBMPlexSans-Text.otf",64,true,true);
    movingBounds            = ofRectangle(0,0,SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    waitTimeForFullscreen   = ofGetElapsedTimeMillis();
    isSecondaryFullscreen   = false;
    isSecondaryWinLoaded    = false;
    generativeState         = 0;
    displacementX           = 0;

    // ---------------------------------------------- VIDEO
    mainVideo               = new ofVideoPlayer();
    rgb                     = new ofImage();
    finalTexture            = new ofFbo();
    correctedFinalTexture   = new ofFbo();
    colorCorrection         = new ofShader();
    warpManager             = new ofxWarpController();

    //mainVideo->load("videos/videoMatrox.mov");
    //mainVideo->setLoopState(OF_LOOP_NORMAL);
    //mainVideo->play();

    rgb->load("videos/rgb.jpg");
    finalTexture->allocate(MAIN_SCREEN_W,MAIN_SCREEN_H,GL_RGBA,2);
    correctedFinalTexture->allocate(MAIN_SCREEN_W,MAIN_SCREEN_H,GL_RGBA,2);
    colorCorrection->load("videos/colorBasic");

    warpManager->loadSettings("videos/warpingSetting.json");
    /*std::shared_ptr<ofxWarpPerspectiveBilinear> warp;
    warp = warpManager->buildWarp<ofxWarpPerspectiveBilinear>();
    warp->setSize(MAIN_SCREEN_W,MAIN_SCREEN_H);
    warp->setEdges(glm::vec4(0.03f, 0.03f, 0.03f, 0.03f));
    warp->setExponent(1.2f);
    warp->setNumControlsX(4);*/
    movingWarpManager->loadSettings("videos/movingWarpingSetting.json");

    // ---------------------------------------------- AUDIO

    // ---------------------------------------------- GENERATIVE
    words = {"facultad", "universidad", "democracia", "Valencia", "reunión", "lucha", "coordinadora", "derecho", "estudiantes", "protesta", "declaración", "gobierno", "organización", "sindical", "delegado", "brigada", "sociales", "policía", "representantes", "compañeras", "tensión", "libertad", "cultura", "reforma", "desarrollo", "trabajador", "intelectual", "becas", "ponencia", "coordinación", "autonomía", "institución", "distritos", "flexibilidad", "asamblea", "delegadas", "activista", "grises", "resistencia", "poesía"};

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

    message                     = "";
    messageBuffer               = "";
    _startImuHeading            = 0;
    _imuHeading                 = 0;
    firstValueRead              = false;
    readingCounter              = 0;


    // ----------------------------------------------
    // let something load in loop (GL stuff)
    preload = false; // last setup line
}

//--------------------------------------------------------------
void ofApp::update(){

    //mainVideo->update();

    // --------------------------------------------------- Motion Detection (webcam as activation sensor)
    motionDetection();

    // --------------------------------------------------- Serial Read (data from arduino)
    serialRead();

    // --------------------------------------------------- System sleep mode control
    sleepCountdown();

}

//--------------------------------------------------------------
void ofApp::draw(){
    ofClear(0);
    //-------------------------------------
    if(!preload){
        preload = true;
        mainWindow->toggleFullscreen();
    }
    //-------------------------------------

    // background
    ofBackground(0);

    // RELOAD
    finalTexture->begin();
    ofClear(0,0,0,255);
    if(isSystemSleeping){
        rgb->draw(0,0,MAIN_SCREEN_W,MAIN_SCREEN_H);
    }else{
        ofSetColor(255);
        rgb->draw(0,0,MAIN_SCREEN_W,MAIN_SCREEN_H);
        //mainVideo->draw(0,0,MAIN_SCREEN_W,MAIN_SCREEN_H);
    }
    finalTexture->end();

    correctedFinalTexture->begin();
    ofClear(0,0,0,255);
    colorCorrection->begin();
    colorCorrection->setUniformTexture("tex0",finalTexture->getTexture(),1);
    colorCorrection->setUniform1f("param1f0", 0.0f); // contrast        0 - 10
    colorCorrection->setUniform1f("param1f1", 0.0f); // brightness      0 - 10
    colorCorrection->setUniform1f("param1f2", 5.0f); // saturation      0 - 10

    ofSetColor(255,255);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
    glTexCoord2f(MAIN_SCREEN_W, 0); glVertex3f(MAIN_SCREEN_W, 0, 0);
    glTexCoord2f(MAIN_SCREEN_W, MAIN_SCREEN_H); glVertex3f(MAIN_SCREEN_W, MAIN_SCREEN_H, 0);
    glTexCoord2f(0,MAIN_SCREEN_H);  glVertex3f(0,MAIN_SCREEN_H, 0);
    glEnd();

    colorCorrection->end();
    correctedFinalTexture->end();

    warpManager->getWarp(0)->draw(correctedFinalTexture->getTexture());

    // GUI
    drawGUI();

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
    warpManager->saveSettings("videos/warpingSetting.json");
    movingWarpManager->saveSettings("videos/movingWarpingSetting.json");
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){

}

//--------------------------------------------------------------
void ofApp::initAppDataFolder(){

    const char *homeDir = getenv("HOME");

    if(!homeDir){
        struct passwd* pwd;
        pwd = getpwuid(getuid());
        if (pwd){
            homeDir = pwd->pw_dir;
        }
    }

    string _APPDataPath(homeDir);

    _APPDataPath += "/Documents/RELOAD/data";

    std::filesystem::path tempPath(_APPDataPath.c_str());

    ofSetDataPathRoot(tempPath); // tell OF to look for resources here
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
                if(!firstValueRead && readingCounter > 25){
                    firstValueRead = true;
                    _startImuHeading = _imuHeading;
                }

                if(readingCounter < 30){
                    readingCounter++;
                }

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

    warpManager->onKeyPressed(key);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    warpManager->onKeyReleased(key);
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    warpManager->onMouseMoved(x,y);

    // TESTING (waiting for IMU)
    displacementX = x;
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    warpManager->onMouseDragged(x,y);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    warpManager->onMousePressed(x,y);
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    warpManager->onMouseReleased(x,y);
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}

//--------------------------------------------------------------
void ofApp::updateMovingWindow(ofEventArgs &e){

}

//--------------------------------------------------------------
void ofApp::drawMovingWindow(ofEventArgs &e){

    if(ofGetElapsedTimeMillis() - waitTimeForFullscreen > 1000 && !isSecondaryWinLoaded){
        isSecondaryWinLoaded = true;
        movingWindow->toggleFullscreen();
        isSecondaryFullscreen = !isSecondaryFullscreen;
    }

    // background
    ofBackground(0);

    movingFBO->begin();
    ofClear(0);
    ofSetColor(255);
    drawTextureCropInsideRect(&finalTexture->getTexture(),-displacementX,0,MAIN_SCREEN_W,MAIN_SCREEN_H,movingBounds,false);
    movingFBO->end();

    glitchedFBO->begin();
    ofSetColor(255,120);
    fboGlitch->draw(*movingFBO,0,0,SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    ofSetColor(255,255);
    switch (generativeState) {
    case 0:

        break;
    case 1: // numeros

        break;
    case 2: // palabras

        break;
    case 3: // cuadrados (sound buffer)

        break;
    default:
        break;
    }
    glitchedFBO->end();

    finalMovingFbo->begin();
    ofClear(0,0,0,255);
    desaturate->begin();
    desaturate->setUniformTexture("tex0",glitchedFBO->getTexture(),1);
    desaturate->setUniform1f("param1f0", 4.0f); // bleach           0 - 5
    desaturate->setUniform1f("param1f1", 9.0f); // desaturation     0 - 10

    ofSetColor(255,255);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
    glTexCoord2f(SECONDARY_SCREEN_W, 0); glVertex3f(SECONDARY_SCREEN_W, 0, 0);
    glTexCoord2f(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H); glVertex3f(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H, 0);
    glTexCoord2f(0,SECONDARY_SCREEN_H);  glVertex3f(0,SECONDARY_SCREEN_H, 0);
    glEnd();

    desaturate->end();
    finalMovingFbo->end();

    movingWarpManager->getWarp(0)->draw(finalMovingFbo->getTexture());

}

//--------------------------------------------------------------
void ofApp::keyPressedMovingWindow(ofKeyEventArgs &e){
    if(e.key == 'f'){
        movingWindow->toggleFullscreen();
        isSecondaryFullscreen = !isSecondaryFullscreen;
        if(!isSecondaryFullscreen){
            movingWindow->setWindowShape(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
        }
    }

    movingWarpManager->onKeyPressed(e.key);
}

//--------------------------------------------------------------
void ofApp::keyReleasedMovingWindow(ofKeyEventArgs &e){
    movingWarpManager->onKeyReleased(e.key);
}

//--------------------------------------------------------------
void ofApp::mouseMovedMovingWindow(ofMouseEventArgs &e){
    movingWarpManager->onMouseMoved(e.x,e.y);
}

//--------------------------------------------------------------
void ofApp::mouseDraggedMovingWindow(ofMouseEventArgs &e){
    movingWarpManager->onMouseDragged(e.x,e.y);
}

//--------------------------------------------------------------
void ofApp::mousePressedMovingWindow(ofMouseEventArgs &e){
    movingWarpManager->onMousePressed(e.x,e.y);
}

//--------------------------------------------------------------
void ofApp::mouseReleasedMovingWindow(ofMouseEventArgs &e){
    movingWarpManager->onMouseReleased(e.x,e.y);
}
