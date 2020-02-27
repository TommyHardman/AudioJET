#pragma once

#include "ofMain.h"
#include "ofxFft.h"
#include "ofxMidiFile.h"
#include "ofxGui.h"

enum {SINE, MIC, NOISE};

class ofApp : public ofBaseApp {
public:
	void setup();
    void timePeaks(vector<float> buffer, float scale, float offset, float threshold);
    void freqPeaks(vector<float> buffer, float scale, float offset, float threshold);
	void plot(vector<float>& buffer, float scale, float offset, bool freq);
	void audioReceived(float* input, int bufferSize, int nChannels);
	void draw();
	void keyPressed(int key);
    
    void shuffle_midiNotes();
    void shuffle_timeMarkers();
    int roundUp(int numToRound, int multiple);
    
    void exportMidi();

	int plotHeight, bufferSize;
    
    ofSoundStream soundStream;

	ofxFft* fft;
    
    bool isActive = true;
	
	int mode;
	
	int appWidth, appHeight;
	
	ofMutex soundMutex;
	vector<float> drawBins, middleBins, audioBins;
	vector<float> drawBuffer, middleBuffer, audioBuffer;
    
    // Midi
    ofxMidiFile midiFile;
    vector<int> midiNotes;
    vector<float> timeMarkers;
    
    bool PAUSE = false;
    
    
    // Gui
    ofxPanel gui;
    ofxFloatSlider freq_threshold, time_threshold;
};
