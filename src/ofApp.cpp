#include "ofApp.h"

//------------------------------------------------------------
float powFreq(float i) {
    return powf(i, 3);
}

//------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);

    ofSoundStreamListDevices();
    
    plotHeight = 128;
    fft.setup(16384);
    
    midiNotes.resize(0);

    deleteMidiFile();
    
    // Gui
    gui.setup();
    gui.add(time_threshold.setup("time threshold",64, -plotHeight/2, plotHeight/2));
    gui.add(freq_threshold.setup("freq threshold",100,0, plotHeight));
    gui.add(generate.setup("Generate Midi",200,30));
    generate.addListener(this, &ofApp::exportMidi);
    gui.add(auto_generate.setup("Auto generate Midi",200,30));
    auto_generate.addListener(this, &ofApp::autoGenerate);
    gui.setPosition(ofGetWidth()/2 - gui.getWidth()/2, ofGetHeight()/2 + 2.5 * gui.getHeight()/2);

    // Title text
    verdana24.load("verdana.ttf", 24, true, true);
    verdana24.setLineHeight(34.0f);
    verdana24.setLetterSpacing(2.035);
    
    // Info text
    verdana16.load("verdana.ttf", 10, true, true);
    verdana16.setLineHeight(34.0f);
    verdana16.setLetterSpacing(1.035);
    
}

//------------------------------------------------------------
void ofApp::update() {
    fft.update();
    // calculate peaks
    freqPeaks(fft.getBins(), plotHeight, freq_threshold);
    timePeaks(fft.getAudio(), plotHeight/2, time_threshold);
}

//------------------------------------------------------------
void ofApp::plot(vector<float>& buffer, float scale, float threshold) {
    ofNoFill();
    int n = MIN(1024, buffer.size());

    ofPushMatrix();
    ofTranslate(ofGetWidth()/2 - n/2, scale);
    ofSetColor(255, 0, 0);
    ofDrawLine(0, -threshold,n , -threshold); // threshold line

    ofBeginShape();
    for (int i = 0; i < n; i++) {
        ofSetColor(255, 255, 255); // white signal
        ofVertex(i, buffer[i] * -scale);
        if (buffer[i] * scale > threshold) {
            ofSetColor(255, 0, 0); // red thresholding
            ofDrawCircle(i,buffer[i] * -scale,10);
        }
    }
    ofEndShape();
    ofPopMatrix();
}

//------------------------------------------------------------
void ofApp::deleteMidiFile() {
    midiFile.clear(); // clear file and save
    midiFile.save("midi.mid");
}


//------------------------------------------------------------
void ofApp::timePeaks(vector<float>& buffer, float scale, float threshold) { // updates time interval vector
    int n = MIN(1024, buffer.size());
    
    int peaks = 0;
    timeMarkers.clear();
    for (int i = 0; i < n; i++) {
        if (buffer[i] * scale > threshold) {
            peaks++;
            float t = (i * (1.0/fft.stream.getSampleRate())) * 1000; // time (ms)
            
            timeMarkers.resize(peaks);
            timeMarkers.push_back(t);
        }
    }
}

//------------------------------------------------------------
void ofApp::freqPeaks(vector<float>& buffer, float scale, float threshold) { // updates frequency peak vector and calculates midiNotes
    int n = MIN(1024, buffer.size());
    
    int peaks = 0;
    midiNotes.clear();
    for (int i = 0; i < n; i++) {
        if (buffer[i] * scale > threshold) {
            peaks++;
        
            float f = ((fft.stream.getSampleRate() * 0.5) / buffer.size()) * i;                      // frequency (Hz)
            int midiNote = 12 * log2(f / 440.0) + 69;    // midi
            if (midiNote > 0) {
                midiNotes.resize(peaks);
                midiNotes.push_back(midiNote);
            }
            
        }
    }
   
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
void ofApp::autoGenerate() {
    // random thresholds
    time_threshold = ofRandom(-plotHeight/2, plotHeight/2);
    freq_threshold = ofRandom(0, plotHeight);
    
    exportMidi();
}


//------------------------------------------------------------
void ofApp::exportMidi() {
    
    
    if ((midiNotes.size() < 1) || (timeMarkers.size() < 2)) {
        // abort export
        ofSystemAlertDialog("Midi Export aborted :: No midiNotes or timeMarkers detected - please adjust thresholds");
    } else {

        // clear previous file
        deleteMidiFile();
        
        shuffle_midiNotes();
        shuffle_timeMarkers();
        
        float chunkTime = ((fft.getAudioBufferSize() * 4 * 1000)) / fft.stream.getSampleRate(); // max signal length in milliseconds
        
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
        ofSystemAlertDialog("Midi Export successful :)");
    }
}

//------------------------------------------------------------
void ofApp::draw() {
    ofBackground(50);
	ofSetColor(255);
    
	ofPushMatrix();
	ofTranslate(16, 16);
    plot(fft.getAudio(), plotHeight/2, time_threshold); // draw time signal
    
	ofTranslate(0, plotHeight + 16);
    plot(fft.getBins(), plotHeight, freq_threshold); // draw frequency signal
    ofPopMatrix();
    
    
    if (PAUSE) {
        ofSetColor(255);
        ofDrawBitmapString("Midi Notes :", 700, 200);
        for (int i = 1; i < midiNotes.size(); i++) { // first index is 0
            ofDrawBitmapString(ofToString(midiNotes[i]),700, 200 + (i * 15));
            // export to midi file
        }
        ofDrawBitmapString("Time markers :", 900, 200);
        for (int i = 1; i < timeMarkers.size(); i++) {
            if (i < 30) {
                ofDrawBitmapString(ofToString(timeMarkers[i]),900, 200 + (i * 15));
            }
            
        }
    }
    
    // Titles & Gui
    ofPushMatrix();
    ofSetColor(255,255 * sin(ofGetElapsedTimef() * 10));
    ofRectangle rect = verdana24.getStringBoundingBox(TITLE, ofGetWidth()/2, ofGetHeight()/2);
    verdana24.drawString(TITLE, ofGetWidth()/2 - rect.width/2, ofGetHeight()/2 - rect.height/2);
    ofPopMatrix();
    
    ofPushMatrix();
    ofSetColor(200);
    ofRectangle rect2 = verdana16.getStringBoundingBox(INFO, ofGetWidth()/2, ofGetHeight()/2);
    verdana16.drawString(INFO, ofGetWidth()/2 - rect2.width/2, ofGetHeight()/2 - rect2.height/2 + 100);
    ofPopMatrix();
    
    gui.draw();
	
}



//------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    cout << ofVec2f(x,y) << endl;
}

//------------------------------------------------------------
void ofApp::keyPressed(int key) {
	
    if (key == ' ') { // snapshot of audio
        PAUSE = !PAUSE;
        if (PAUSE) {
            fft.stopSoundStream();
            cout << "... Stopping soundStream ..." << endl;
        } else {
            fft.startSoundStream();
            cout << "... Starting soundStream ..." << endl;
        }
    }

    if (key == 'e') { // export midi file
        cout << "Exporting midi file" << endl;
        exportMidi();
    }
    
}
