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
    
    midiFile.addNoteOn(60, 100, 0);
    midiFile.addNoteOff(60, 100, 384);
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
    
    
	ofTranslate(0, plotHeight + 16);
	ofDrawBitmapString("Frequency Domain", 0, 0);
	plot(drawBins, -plotHeight, plotHeight / 2, true);
    ofPopMatrix();
    
    ofPushMatrix();
    ofTranslate(16,plotHeight + 16 + 16);
    // draw circle at highest peaks
    freqPeaks(drawBins, -plotHeight, plotHeight / 2, 100);
    
    ofPopMatrix();
    
    if (PAUSE) {
        ofSetColor(255);
        for (int i = 1; i < midiNotes.size(); i++) { // first index is 0
            ofDrawBitmapString(ofToString(midiNotes[i]),ofGetWidth()/2 + i*50 , ofGetHeight() - 200);
            // export to midi file
        }
    }
    
	
}

//------------------------------------------------------------
float powFreq(float i) {
	return powf(i, 3);
}


//------------------------------------------------------------
void ofApp::freqPeaks(vector<float> buffer, float scale, float offset, float threshold) {
    int n = buffer.size();
    int factor = 10;
    glPushMatrix();
    glTranslatef(0, plotHeight/2 + offset, 0);
    ofSetColor(255, 0, 0);
    ofDrawLine(0, -threshold,(n/16) * factor,-threshold); // threshold line
    
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
            
            ofDrawCircle(i*factor, buffer[i] * scale, 10);
            ofDrawBitmapString(ofToString(f),i*factor, 40); // frequencies

            
        }
    }
    glPopMatrix();
}

//------------------------------------------------------------
void ofApp::plot(vector<float>& buffer, float scale, float offset, bool freq) {
	ofNoFill();
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
        ofDrawRectangle(0, 0,n, plotHeight);
        glPushMatrix();
        glTranslatef(0, plotHeight / 2 + offset, 0);
        ofBeginShape();
        for (int i = 0; i < n; i++) {
            ofVertex(i, buffer[i] * scale);
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
    
    if (key == 'p') {
        PAUSE = !PAUSE;
        cout << "PAUSE : " + ofToString(PAUSE) << endl;
    }

}
