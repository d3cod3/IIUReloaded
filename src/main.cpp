#include "ofMain.h"
#include "ofApp.h"

#include "ofAppGLFWWindow.h"

#include "config.h"

//========================================================================
int main( ){

    ofGLFWWindowSettings settings;
    settings.setGLVersion(2, 1);
    settings.stencilBits = 0;
    settings.setSize(STANDARD_WINDOW_W, STANDARD_WINDOW_H);
    settings.setPosition(ofVec2f(0,0));
    settings.resizable = true;
    settings.decorated = true;

    // MAIN WINDOW
    shared_ptr<ofAppGLFWWindow> mainWindow = dynamic_pointer_cast<ofAppGLFWWindow>(ofCreateWindow(settings));
    shared_ptr<ofApp> mainApp(new ofApp);

    ofRunApp(mainWindow, mainApp);
    ofRunMainLoop();

        // done
    return EXIT_SUCCESS;

}
