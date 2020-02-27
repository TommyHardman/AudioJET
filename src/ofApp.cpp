#include "ofApp.h"

#define FS 44100

//------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);

	plotHeight = 128;
	bufferSize = 2048;
    
    ofSoundStreamListDevices();

	fft = ofxFft::create(bufferSize, OF_FFT_WINDOW_HAMMING);
	// To use FFTW, try:
	//fft = ofxFft::create(bufferSize, OF_FFT_WINDOW_HAMMING, OF_FFT_FFTW);


	drawBuffer.resize(bufferSize);
	middleBuffer.resize(bufferSize);
	audioBuffer.resize(bufferSize);
	
	drawBins.resize(fft->getBinSize());
	middleBins.resize(fft->getBinSize());
	audioBins.resize(fft->getBinSize());
    
    midiNotes.resize(0);

	// 0 output channels,
	// 1 input channel
	// 44100 samples per second
	// [bins] samples per buffer
	// 4 num buffers (latency)
    
    soundStream.setDeviceID(0);
    
	//ofSoundStreamSetup(0, 1, this, 44100, bufferSize, 4);
    soundStream.setup(this, 0, 1, FS, bufferSize, 4);
    
	mode = MIC;
	appWidth = ofGetWidth();
	appHeight = ofGetHeight();

	ofBackground(0, 0, 0);
    
    midiFile.clear(); // clear file and save
    midiFile.save("midi.mid");
    
    // Gui
    gui.setup();
    gui.add(time_threshold.setup("time threshold",-20, plotHeight/2, -plotHeight/2));
    gui.add(freq_threshold.setup("freq threshold",100,0, plotHeight));
    
}

//------------------------------------------------------------
void ofApp::shuffle_midiNotes() {
    std::random_shuffle(midiNotes.begin()+1, midiNotes.end()); // +1 to ignore first 0 value
}

//------------------------------------------------------------
void ofApp::shuffle_timeMarkers() {
    std::random_shuffle(timeMarkers.begin()+1, timeMarkers.end()); // +1 to ignore first 0 value
}

//------------------------------------------------------------
int ofApp::roundUp(int numToRound, int multiple) {
    if (multiple == 0)
        return numToRound;
    
    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;
    
    return numToRound + multiple - remainder;
}

//------------------------------------------------------------
void ofApp::exportMidi() {
    shuffle_midiNotes();
    shuffle_timeMarkers();
    
    float chunkTime = (bufferSize * 1000) / FS; // milliseconds
    
    int interval_map = 0;
    for (int i = 1; i < midiNotes.size(); i++) { // first element is 0
        
        float interval = abs(timeMarkers[i] - timeMarkers[i-1]); // random interval between two time peaks
        int tmp_val = (int)ofMap(interval,0,chunkTime,47,384); // limits ?
        tmp_val = roundUp(tmp_val, 96);
        
     
        midiFile.addNoteOn(midiNotes[i], 127, interval_map);
        midiFile.addNoteOff(midiNotes[i], 127, interval_map + tmp_val);
        interval_map += tmp_val;
        
        cout << tmp_val << endl;
    }
    
    midiFile.save("midi.mid");
}

//------------------------------------------------------------
void ofApp::draw() {
	ofSetColor(255);
	ofPushMatrix();
	ofTranslate(16, 16);
	ofDrawBitmapString("Time Domain", 0, 0);
	
	soundMutex.lock();
	drawBuffer = middleBuffer;
	drawBins = middleBins;
	soundMutex.unlock();
	
	plot(drawBuffer, plotHeight / 2, 0, false);
    timePeaks(drawBuffer, plotHeight / 2, 0, time_threshold);
    
    
	ofTranslate(0, plotHeight + 16);
	ofDrawBitmapString("Frequency Domain", 0, 0);
	plot(drawBins, -plotHeight, plotHeight / 2, true);
    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(16,plotHeight + 16 + 16);
    // draw circle at highest peaks
    freqPeaks(drawBins, -plotHeight, plotHeight / 2, freq_threshold);
    
    ofPopMatrix();
    
    if (PAUSE) {
        ofSetColor(255);
        for (int i = 1; i < midiNotes.size(); i++) { // first index is 0
            ofDrawBitmapString(ofToString(midiNotes[i]),200 + i*50 , ofGetHeight() - 200);
            // export to midi file
        }
        for (int i = 1; i < timeMarkers.size(); i++) { 
            ofDrawBitmapString(ofToString(timeMarkers[i]),200 + i*50 , ofGetHeight() - 300);
            
        }
    }
    
    ofPushMatrix();
    //ofScale(10);
    ofSetColor(255);
    ofDrawBitmapString("Audio JET", ofGetWidth()/2, ofGetHeight()/2);
    ofPopMatrix();
    
    gui.draw();
	
}

//------------------------------------------------------------
float powFreq(float i) {
	return powf(i, 3);
}

//------------------------------------------------------------
void ofApp::timePeaks(vector<float> buffer, float scale, float offset, float threshold) {
    int n = buffer.size();
  
    glPushMatrix();
    glTranslatef(0, plotHeight/2 + offset, 0);
    ofSetColor(255, 255, 0);
    ofDrawLine(0, threshold, n*0.5, threshold); // threshold line
    
    int peaks = 0;
    timeMarkers.clear();
    for (int i = 0; i < n; i++) {
        float val = (buffer[i] * scale);
        if (val < threshold) {
            peaks++;
            //cout << val << endl;
            
            float t = (i * (1.0/FS)) * 1000; // time (ms)
          
            timeMarkers.resize(peaks);
            timeMarkers.push_back(t);

            ofDrawCircle(i*0.5, buffer[i] * scale, 5);
            ofDrawBitmapString(ofToString(t),i*0.5, 600); // times
            
            
        }
    }
    glPopMatrix();
}

//------------------------------------------------------------
void ofApp::freqPeaks(vector<float> buffer, float scale, float offset, float threshold) {
    int n = buffer.size();
    int factor = 10;
    glPushMatrix();
    glTranslatef(0, plotHeight/2 + offset, 0);
    ofSetColor(255, 0, 0);
    ofDrawLine(0, -threshold,(n/16) * factor, -threshold); // threshold line
    
    int peaks = 0;
    midiNotes.clear();
    for (int i = 0; i < n/16; i++) {
        float val = abs(buffer[i] * scale);
        if (val > threshold) {
            peaks++;
            //cout << val << endl;
            float f = (FS/2)/n * i;                      // frequency (Hz)
            int midiNote = 12 * log2(f / 440.0) + 69;    // midi
            
            midiNotes.resize(peaks);
            midiNotes.push_back(midiNote);
            
            ofDrawCircle(i*factor, buffer[i] * scale, 5);
            ofDrawBitmapString(ofToString(f),i*factor, 40); // frequencies

            
        }
    }
    glPopMatrix();
}

//------------------------------------------------------------
void ofApp::plot(vector<float>& buffer, float scale, float offset, bool freq) {
	ofNoFill();
    ofSetColor(255);
	int n = buffer.size();
    int factor = 10;
    if (freq) {
        ofDrawRectangle(0, 0,factor * n/16, plotHeight);
        glPushMatrix();
        glTranslatef(0, plotHeight / 2 + offset, 0);
        ofBeginShape();
        for (int i = 0; i < n/16; i++) {
            ofVertex(i * factor, buffer[i] * scale);
        }
        ofEndShape();
        glPopMatrix();
    } else {
        ofDrawRectangle(0, 0,n * 0.5, plotHeight);
        glPushMatrix();
        glTranslatef(0, plotHeight / 2 + offset, 0);
        ofBeginShape();
        for (int i = 0; i < n; i++) {
            ofVertex(i*0.5, buffer[i] * scale);
        }
        ofEndShape();
        glPopMatrix();
    }
}

//------------------------------------------------------------
void ofApp::audioReceived(float* input, int bufferSize, int nChannels) {
    
    if (!PAUSE) {
    
        if (mode == MIC) {
            // store input in audioInput buffer
            memcpy(&audioBuffer[0], input, sizeof(float) * bufferSize);
            
            float maxValue = 0;
            for(int i = 0; i < bufferSize; i++) {
                if(abs(audioBuffer[i]) > maxValue) {
                    maxValue = abs(audioBuffer[i]);
                }
            }
            for(int i = 0; i < bufferSize; i++) {
                audioBuffer[i] /= maxValue;
            }
            
        } else if (mode == NOISE) {
            for (int i = 0; i < bufferSize; i++)
                audioBuffer[i] = ofRandom(-1, 1);
        } else if (mode == SINE) {
            for (int i = 0; i < bufferSize; i++)
                audioBuffer[i] = sinf(PI * i * mouseX / appWidth);
        }
        
        fft->setSignal(&audioBuffer[0]);

        float* curFft = fft->getAmplitude();
        memcpy(&audioBins[0], curFft, sizeof(float) * fft->getBinSize());

        float maxValue = 0;
        for(int i = 0; i < fft->getBinSize(); i++) {
            if(abs(audioBins[i]) > maxValue) {
                maxValue = abs(audioBins[i]);
            }
        }
        for(int i = 0; i < fft->getBinSize(); i++) {
            audioBins[i] /= maxValue;
        }
        
        
        soundMutex.lock();
        middleBuffer = audioBuffer;
        middleBins = audioBins;
        soundMutex.unlock();
        
    }
}

//------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
	case 'm':
		mode = MIC;
		break;
	case 'n':
		mode = NOISE;
		break;
	case 's':
		mode = SINE;
		break;
	}
    
    if (key == 'p') { // snapshot of audio
        PAUSE = !PAUSE;
        cout << "PAUSE : " + ofToString(PAUSE) << endl;
    }

    if (key == 'e') { // export midi file
        cout << "Exporting midi file" << endl;
        exportMidi();
    }
    
}
