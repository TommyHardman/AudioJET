#pragma once

#include "ofMain.h"
#include "ofxMidiFile.h"
#include "ofxGui.h"
#include "ofxEasyFft.h"



class ofApp : public ofBaseApp {
public:
	void setup();
    void update();
	void draw();
	void keyPressed(int key);
    void mousePressed(int x, int y, int button);
    
    // fft & calculation
    ofxEasyFft fft;
    void plot(vector<float>& buffer, float scale, float threshold);
    void timePeaks(vector<float>& buffer, float scale, float threshold);
    void freqPeaks(vector<float>& buffer, float scale, float threshold);
    
    void shuffle_midiNotes();
    void shuffle_timeMarkers();
    
    int roundUp(int numToRound, int multiple);
    
    void exportMidi();

    // vars
    int plotHeight;
    
    // Midi
    ofxMidiFile midiFile;
    vector<int> midiNotes;
    vector<float> timeMarkers;
    
    bool PAUSE = false; // stop / start soundStream
    
    // Gui
    ofxPanel gui;
    ofxFloatSlider freq_threshold, time_threshold;
    ofxButton generate;
    
    // Title
    ofTrueTypeFont    verdana30;
    string TITLE = "Audio JET";
};
