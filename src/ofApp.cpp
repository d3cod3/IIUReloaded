#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // ---------------------------------------------- OF Stuff
    ofSetFrameRate(FPS);
    ofEnableAntiAliasing();
    initAppDataFolder();
    ofHideCursor();

    // ---------------------------------------------- Load XML Settings
    settingsLoaded = false;
    if(xmlSettings.loadFile("videos/settings.xml")){
        if (xmlSettings.pushTag("settings")){
            _audio_device           = static_cast<int>(xmlSettings.getValue("audio_device",0));
            _sample_rate            = static_cast<int>(xmlSettings.getValue("sample_rate",0));
            _buffer_size            = static_cast<int>(xmlSettings.getValue("buffer_size",0));
            _output_channels        = static_cast<int>(xmlSettings.getValue("input_channels",0));
            _input_channels         = static_cast<int>(xmlSettings.getValue("output_channels",0));
            ch1_vol                 = static_cast<float>(xmlSettings.getValue("ch1_vol",0.0));
            ch2_vol                 = static_cast<float>(xmlSettings.getValue("ch2_vol",0.0));
            ch3_vol                 = static_cast<float>(xmlSettings.getValue("ch3_vol",0.0));
            ch4_vol                 = static_cast<float>(xmlSettings.getValue("ch4_vol",0.0));
            campana_vol             = static_cast<float>(xmlSettings.getValue("campana_vol",0.0));
            voz_vol                 = static_cast<float>(xmlSettings.getValue("voz_vol",0.0));
            _serial_device          = xmlSettings.getValue("serial_device","");
            _start_imu_angle        = static_cast<int>(xmlSettings.getValue("start_imu_angle",0));
            _end_imu_angle          = static_cast<int>(xmlSettings.getValue("end_imu_angle",0));
            _motion_threshold       = static_cast<float>(xmlSettings.getValue("motion_threshold",0.0));
            _wait_for_sleep_min     = static_cast<int>(xmlSettings.getValue("wait_for_sleep_min",0));
            _glitch_alpha           = static_cast<int>(xmlSettings.getValue("glitch_alpha",0));
            _glitch_bleach          = static_cast<float>(xmlSettings.getValue("glitch_bleach",0.0));
            _glitch_desaturation    = static_cast<float>(xmlSettings.getValue("glitch_desaturation",0.0));

            settingsLoaded = true;
        }
    }

    if(!settingsLoaded){
        ofLog(OF_LOG_ERROR,"ERROR LOADING SETTINGS FILE!!!");
        ofExit();
    }


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


    movingFBO->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H,GL_RGBA,2);
    glitchedFBO->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H,GL_RGBA,2);
    finalMovingFbo->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H,GL_RGBA,2);
    fboGlitch->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    desaturate->load("videos/movingProjectionShader");
    movingBounds            = ofRectangle(0,0,SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    waitTimeForFullscreen   = ofGetElapsedTimeMillis();
    isSecondaryFullscreen   = false;
    isSecondaryWinLoaded    = false;
    generativeState         = 0;
    displacementX           = 0;
    actualQuadrant          = 0;

    // ---------------------------------------------- VIDEO
    mainVideo               = new ofVideoPlayer();
    rgb                     = new ofImage();
    finalTexture            = new ofFbo();
    correctedFinalTexture   = new ofFbo();
    sectorL                 = new ofFbo();
    sectorC                 = new ofFbo();
    sectorR                 = new ofFbo();
    colorCorrection         = new ofShader();
    warpManager             = new ofxWarpController();

    mainVideo->load("videos/videoMatrox.mov");
    mainVideo->setLoopState(OF_LOOP_NORMAL);
    mainVideo->setVolume(voz_vol);
    mainVideo->stop();

    rgb->load("videos/rgb.jpg");
    finalTexture->allocate(MAIN_SCREEN_W,MAIN_SCREEN_H,GL_RGBA,2);
    correctedFinalTexture->allocate(MAIN_SCREEN_W,MAIN_SCREEN_H,GL_RGBA,2);
    sectorL->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    sectorC->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    sectorR->allocate(SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    colorCorrection->load("videos/colorBasic");

    warpManager->loadSettings("videos/warpingSetting.json");
    std::shared_ptr<ofxWarpPerspectiveBilinear> warp1, warp2, warp3;
    /*warp1 = warpManager->buildWarp<ofxWarpPerspectiveBilinear>();
    warp1->setSize(MAIN_SCREEN_W,MAIN_SCREEN_H);
    warp1->setEdges(glm::vec4(0.03f, 0.03f, 0.03f, 0.03f));
    warp1->setExponent(1.2f);

    warp2 = warpManager->buildWarp<ofxWarpPerspectiveBilinear>();
    warp2->setSize(MAIN_SCREEN_W,MAIN_SCREEN_H);
    warp2->setEdges(glm::vec4(0.03f, 0.03f, 0.03f, 0.03f));
    warp2->setExponent(1.2f);

    warp3 = warpManager->buildWarp<ofxWarpPerspectiveBilinear>();
    warp3->setSize(MAIN_SCREEN_W,MAIN_SCREEN_H);
    warp3->setEdges(glm::vec4(0.03f, 0.03f, 0.03f, 0.03f));
    warp3->setExponent(1.2f);*/


    movingWarpManager->loadSettings("videos/movingWarpingSetting.json");

    // ---------------------------------------------- AUDIO
    engine = new pdsp::Engine();
    //engine->listDevices();
    engine->setChannels(_input_channels,_output_channels);
    this->setChannels(_input_channels,0);

    for(int in=0;in<_input_channels;in++){
        engine->audio_in(in) >> this->in(in);
    }
    this->out_silent() >> engine->blackhole();

    engine->setOutputDeviceID(_audio_device);
    engine->setInputDeviceID(_audio_device);
    engine->setup(_sample_rate, _buffer_size, 3);

    audioCH1.out_signal() >> engine->audio_out(0);
    audioCH2.out_signal() >> engine->audio_out(1);
    audioCH3.out_signal() >> engine->audio_out(2);
    audioCH4.out_signal() >> engine->audio_out(3);

    audioCH1.out_signal() >> mix;
    audioCH2.out_signal() >> mix;
    audioCH3.out_signal() >> mix;
    audioCH4.out_signal() >> mix;

    mix >> scope >> engine->blackhole();

    audioCH1_file.load(ofToDataPath("videos/CH1.wav"));
    audioCH2_file.load(ofToDataPath("videos/CH2.wav"));
    audioCH3_file.load(ofToDataPath("videos/CH3.wav"));
    audioCH4_file.load(ofToDataPath("videos/CH4.wav"));

    main_playhead = std::numeric_limits<int>::max();
    main_step = audioCH1_file.samplerate() / _sample_rate;
    main_playhead = 0.0;

    short *shortBuffer = new short[_buffer_size];
    for (int i = 0; i < _buffer_size; i++){
        shortBuffer[i] = 0;
    }
    ofSoundBuffer tmpBuffer(shortBuffer,static_cast<size_t>(_buffer_size),1,static_cast<unsigned int>(_sample_rate));
    audioCH1_buffer.clear();
    audioCH1_buffer = tmpBuffer;
    audioCH2_buffer.clear();
    audioCH2_buffer = tmpBuffer;
    audioCH3_buffer.clear();
    audioCH3_buffer = tmpBuffer;
    audioCH4_buffer.clear();
    audioCH4_buffer = tmpBuffer;
    campana_buffer.clear();
    campana_buffer = tmpBuffer;

    emptyBuffer = tmpBuffer;

    campanaFile.load(ofToDataPath("videos/campana.wav"));

    campana_vol             = 1.0f;

    randomCHCamapana        = 0;
    playCampana             = false;
    campana_playhead        = std::numeric_limits<int>::max();
    campana_step            = campanaFile.samplerate() / _sample_rate;
    campana_playhead        = 0.0;

    // ---------------------------------------------- GENERATIVE
    words = {"facultad", "universidad", "democracia", "Valencia", "reunión", "lucha", "coordinadora", "derecho", "estudiantes", "protesta", "declaración", "gobierno", "organización", "sindical", "delegado", "brigada", "sociales", "policía", "representantes", "compañeras", "tensión", "libertad", "cultura", "reforma", "desarrollo", "trabajador", "intelectual", "becas", "ponencia", "coordinación", "autonomía", "institución", "distritos", "flexibilidad", "asamblea", "delegadas", "activista", "grises", "resistencia", "poesía"};

    font                = new ofxTrueTypeFontUC();
    font->load("videos/IBMPlexSans-Text.otf",64,true,true);
    resetWordTime       = ofGetElapsedTimeMillis();
    waitForWord         = static_cast<int>(floor(ofRandom(40,1000)));
    wordCounter         = 0;

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
    waitTime                    = 1000*60*_wait_for_sleep_min;
    isSystemSleeping            = false;

    // ---------------------------------------------- IMU from serial port (arduino)
    serialAttached = false;
    //serial.listDevices();
    if(serial.setup(_serial_device.c_str(), 9600)){
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

    // --------------------------------------------------- GENERATIVE
    if(!isSystemSleeping && mainVideo->isLoaded()){
        mainVideo->update();

        // Campana trigger
        if(mainVideo->getCurrentFrame() == 1618 || mainVideo->getCurrentFrame() == 5618 || mainVideo->getCurrentFrame() == 6395 || mainVideo->getCurrentFrame() == 10285 || mainVideo->getCurrentFrame() == 11755 || mainVideo->getCurrentFrame() == 15751){
            playRandomCampana();
        }

        if(actualQuadrant != 0 && actualQuadrant != 4){
            if(mainVideo->getCurrentFrame() >= 0 && mainVideo->getCurrentFrame() < 1618){
                generativeState = 1; // numeros
            }else if(mainVideo->getCurrentFrame() >= 1618 && mainVideo->getCurrentFrame() < 5618){
                if(actualQuadrant == 1){
                    generativeState = 3; // cuadrados
                }else if(actualQuadrant == 2){
                    generativeState = 2; // palabras
                }else if(actualQuadrant == 3){
                    generativeState = 1; // numeros
                }
            }else if(mainVideo->getCurrentFrame() >= 5618 && mainVideo->getCurrentFrame() < 6395){
                generativeState = 2; // palabras
            }else if(mainVideo->getCurrentFrame() >= 6395 && mainVideo->getCurrentFrame() < 10285){
                if(actualQuadrant == 1){
                    generativeState = 1; // numeros
                }else if(actualQuadrant == 2){
                    generativeState = 3; // cuadrados
                }else if(actualQuadrant == 3){
                    generativeState = 2; // palabras
                }
            }else if(mainVideo->getCurrentFrame() >= 10285 && mainVideo->getCurrentFrame() < 11755){
                generativeState = 3; // cuadrados
            }else if(mainVideo->getCurrentFrame() >= 11755 && mainVideo->getCurrentFrame() < 15751){
                if(actualQuadrant == 1){
                    generativeState = 2; // palabras
                }else if(actualQuadrant == 2){
                    generativeState = 1; // numeros
                }else if(actualQuadrant == 3){
                    generativeState = 3; // cuadrados
                }
            }

        }else{
            generativeState = 0; // void
        }
    }else{
        generativeState = 0; // void
    }




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
        if(mainVideo->isLoaded()){
            mainVideo->setVolume(voz_vol);
            mainVideo->play();
        }
    }
    //-------------------------------------

    // background
    ofBackground(0);

    // RELOAD
    finalTexture->begin();
    ofClear(0,0,0,255);
    if(!isSystemSleeping && !drawGui && mainVideo->isLoaded()){
        ofSetColor(255);
        mainVideo->draw(0,0,MAIN_SCREEN_W,MAIN_SCREEN_H);
    }else{
        rgb->draw(0,0,MAIN_SCREEN_W,MAIN_SCREEN_H);
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

    sectorL->begin();
    drawTextureCropInsideRect(&correctedFinalTexture->getTexture(),0,0,MAIN_SCREEN_W,MAIN_SCREEN_H,movingBounds,false);
    sectorL->end();

    sectorC->begin();
    drawTextureCropInsideRect(&correctedFinalTexture->getTexture(),-SECONDARY_SCREEN_W,0,MAIN_SCREEN_W,MAIN_SCREEN_H,movingBounds,false);
    sectorC->end();

    sectorR->begin();
    drawTextureCropInsideRect(&correctedFinalTexture->getTexture(),-SECONDARY_SCREEN_W*2,0,MAIN_SCREEN_W,MAIN_SCREEN_H,movingBounds,false);
    sectorR->end();

    warpManager->getWarp(0)->draw(sectorL->getTexture());
    warpManager->getWarp(1)->draw(sectorC->getTexture());
    warpManager->getWarp(2)->draw(sectorR->getTexture());

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

    engine->setChannels(0,0);
    delete engine;
}

//--------------------------------------------------------------
void ofApp::audioProcess(float *input, int bufferSize, int nChannels){
    if(audioCH1_file.loaded() && audioCH2_file.loaded() && audioCH3_file.loaded() && audioCH4_file.loaded() && mainVideo->isLoaded() && mainVideo->isPlaying() && !drawGui){
        // assuming the 4 audio file are exatcly the same length!!!
        for(size_t i = 0; i < audioCH1_buffer.getNumFrames(); i++) {
            int n = static_cast<int>(floor(main_playhead));

            if(n < audioCH1_file.length()-1){
                float fract = static_cast<float>(main_playhead - n);
                float isample1 = audioCH1_file.sample(n, 0)*(1.0f-fract) + audioCH1_file.sample(n+1, 0)*fract;
                float isample2 = audioCH2_file.sample(n, 0)*(1.0f-fract) + audioCH2_file.sample(n+1, 0)*fract;
                float isample3 = audioCH3_file.sample(n, 0)*(1.0f-fract) + audioCH3_file.sample(n+1, 0)*fract;
                float isample4 = audioCH4_file.sample(n, 0)*(1.0f-fract) + audioCH4_file.sample(n+1, 0)*fract;
                audioCH1_buffer.getSample(i,0) = isample1 * ch1_vol;
                audioCH2_buffer.getSample(i,0) = isample2 * ch2_vol;
                audioCH3_buffer.getSample(i,0) = isample3 * ch3_vol;
                audioCH4_buffer.getSample(i,0) = isample4 * ch4_vol;

                main_playhead += main_step;

            }else{
                main_playhead = 0.0;
            }
        }

        if(campanaFile.loaded() && playCampana){
            for(size_t i = 0; i < campana_buffer.getNumFrames(); i++) {
                int nc = static_cast<int>(floor(campana_playhead));

                if(nc < campanaFile.length()-1){
                    float cfract = static_cast<float>(campana_playhead - nc);
                    float isamplec = campanaFile.sample(nc, 0)*(1.0f-cfract) + campanaFile.sample(nc+1, 0)*cfract;
                    campana_buffer.getSample(i,0) = isamplec * campana_vol;

                    campana_playhead += campana_step;
                }else{
                    playCampana = false;
                }
            }
            campana.copyInput(campana_buffer.getBuffer().data(),campana_buffer.getNumFrames());
        }else{
            campana_buffer  = emptyBuffer;
        }

    }else{
        audioCH1_buffer = emptyBuffer;
        audioCH2_buffer = emptyBuffer;
        audioCH3_buffer = emptyBuffer;
        audioCH4_buffer = emptyBuffer;

    }

    audioCH1.copyInput(audioCH1_buffer.getBuffer().data(),audioCH1_buffer.getNumFrames());
    audioCH2.copyInput(audioCH2_buffer.getBuffer().data(),audioCH2_buffer.getNumFrames());
    audioCH3.copyInput(audioCH3_buffer.getBuffer().data(),audioCH3_buffer.getNumFrames());
    audioCH4.copyInput(audioCH4_buffer.getBuffer().data(),audioCH4_buffer.getNumFrames());

    unique_lock<mutex> lock(audioMutex);

}

//--------------------------------------------------------------
void ofApp::playRandomCampana(){
    randomCHCamapana = static_cast<int>(floor(ofRandom(0,4)));

    campana.disconnectOut();

    campana.out_signal() >> engine->audio_out(randomCHCamapana);

    campana_playhead    = 0.0;
    playCampana         = true;
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
                if(mainVideo->isLoaded()){
                    mainVideo->setVolume(voz_vol);
                    mainVideo->play();
                }
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

                displacementX = ofClamp(ofMap(_imuHeading,_start_imu_angle,_end_imu_angle,-1280,3900),-1280,3900);

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
    if(mainVideo->isLoaded()){
        if(ofGetElapsedTimeMillis()-resetTime > waitTime && mainVideo->getPosition()>0.99 && !isSystemSleeping){
            isSystemSleeping = true;
            mainVideo->stop();
        }
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == 'f'){
        mainWindow->toggleFullscreen();
    }else if(key == 'g'){
        drawGui = !drawGui;
        if(drawGui){
            ofShowCursor();
        }else{
            ofHideCursor();
        }
    }else if(key == 'c'){
        playRandomCampana();
    }else if(key == 'p'){
        mainVideo->setPaused(true);
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
    if(displacementX < 0){
        actualQuadrant = 0; // outside left
    }else if(displacementX >= 0 && displacementX < 1280){
        actualQuadrant = 1; // q1
    }else if(displacementX >= 1280 && displacementX < 2560){
        actualQuadrant = 2; // q2
    }else if(displacementX >= 2560 && displacementX < 3840){
        actualQuadrant = 3; // q3
    }else if(displacementX >= 3840){
        actualQuadrant = 4; // outside right
    }
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
    ofSetColor(255,_glitch_alpha);
    drawTextureCropInsideRect(&finalTexture->getTexture(),-displacementX,0,MAIN_SCREEN_W,MAIN_SCREEN_H,movingBounds,false);
    ofSetColor(255,255);
    switch (generativeState) {
    case 0:
        // void
        break;
    case 1: // numeros
        drawNumeros();
        break;
    case 2: // palabras
        drawPalabras();
        break;
    case 3: // cuadrados (sound buffer)
        drawCuadrados();
        break;
    case 4:
        // void
        break;
    default:
        break;
    }
    movingFBO->end();

    glitchedFBO->begin();
    if(!isSystemSleeping && !drawGui){
        ofSetColor(255,255);
        fboGlitch->draw(*movingFBO,0,0,SECONDARY_SCREEN_W, SECONDARY_SCREEN_H);
    }else{
        ofClear(0);
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
void ofApp::drawNumeros(){
    for(size_t i = 0; i < scope.getBuffer().size(); i++) {
        if(i%24 == 0){
            float sample = scope.getBuffer().at(i);
            float x = ofMap(i, 0, scope.getBuffer().size(), 0, SECONDARY_SCREEN_W);
            float y = ofMap(hardClip(sample), -1, 1, 0, SECONDARY_SCREEN_H);

            ofPushMatrix();
            ofTranslate(x,y);
            ofScale(hardClip(sample));
            font->drawString(ofToString(sample),0,0);
            ofPopMatrix();
        }
    }
}

//--------------------------------------------------------------
void ofApp::drawPalabras(){
    if(ofGetElapsedTimeMillis() - resetWordTime > waitForWord){
        resetWordTime           = ofGetElapsedTimeMillis();
        waitForWord             = static_cast<int>(floor(ofRandom(40,1000)));

        wordCounter = static_cast<int>(floor(ofRandom(0,words.size())));
        if(wordCounter == words.size()){
            wordCounter--;
        }
    }

    ofSetColor(250);
    float stringWidth = font->getStringBoundingBox(words.at(wordCounter),SECONDARY_SCREEN_W/2,SECONDARY_SCREEN_H/2).width;
    font->drawString(words.at(wordCounter),SECONDARY_SCREEN_W/2 - stringWidth/2,SECONDARY_SCREEN_H/2);
}

//--------------------------------------------------------------
void ofApp::drawCuadrados(){
    int qw = static_cast<int>(floor(SECONDARY_SCREEN_W / 16));
    int qh = static_cast<int>(floor(SECONDARY_SCREEN_H / 16));

    unique_lock<mutex> lock(audioMutex);

    for(size_t x = 0; x < 16; x++) {
        for(size_t y = 0; y < 16; y++) {
            int loc = x + y*16;
            if(loc<scope.getBuffer().size()){
                ofSetColor(245,ofMap(hardClip(scope.getBuffer().at(loc)),-1,1,0,200));
                ofDrawRectangle(x*qw,y*qh,qw,qh*hardClip(scope.getBuffer().at(loc)));
            }
        }
    }
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
