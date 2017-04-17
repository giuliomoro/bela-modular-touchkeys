/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
  Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
  Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/


#include <Bela.h>
#include <Midi.h>
#include <stdlib.h>
#include <rtdk.h>
#include <cmath>
#include <TouchKeysMidiParser.h>

float gFreq;
float gPhaseIncrement = 0;
bool gIsNoteOn = 0;
int gVelocity = 0;
float gSamplingPeriod = 0;

TouchKeysMidiParser parser;
Midi midi;

const char* gMidiPort0 = "hw:1,0,0";

bool setup(BelaContext *context, void *userData)
{
	midi.readFrom(gMidiPort0);
	midi.writeTo(gMidiPort0);
	midi.enableParser(true);
	midi.setParserCallback(TouchKeysMidiParser::midiMessageCallback, (void*)&parser);
	if(context->analogFrames == 0) {
		rt_printf("Error: this example needs the analog I/O to be enabled\n");
		return false;
	}

	if(context->audioOutChannels < 2 ||
		context->analogOutChannels < 2){
		printf("Error: for this project, you need at least 2 analog and audio output channels.\n");
		return false;
	}

	gSamplingPeriod = 1/context->audioSampleRate;
	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.

float f0;
enum {kVelocity, kNoteOn, kNoteNumber};
void render(BelaContext *context, void *userData)
{
	static int count = 0;
	const std::vector<TouchKeysTouch>& touches = parser.getTouches();
	int cvPerTouch = 4;
	float minNote = 24;
	float pitchCvScale = 0.009;
	int maxTouches = 2;
	int activeTouches = 0;
	static float analogOut[8] = {0};
	static float analogOutOld[8] = {0};
	static bool analogOutSmooth[8] = {1, 1, 1, 0, 1, 1, 1, 0};
	float smooth = 0.99;
	for(unsigned int n = 0; n < touches.size() && activeTouches < maxTouches; ++n)
	{
		const TouchKeysTouch& touch = touches[n];
		if(!touch.enabled)
			continue;
		float xCv = ((touch.note - minNote) + touch.x) * pitchCvScale + 0.5f;
		float yCv = touch.y;
		float zCv = touch.z * 0.5f + 0.5f;
		float gate = 1;
		if(count % 200 == 0){
			rt_printf("writing: %.3f %.3f %.3f %.3f\n", xCv, yCv, zCv, gate);
		}
		analogOut[activeTouches * cvPerTouch + 0] = xCv;
		analogOut[activeTouches * cvPerTouch + 1] = yCv;
		analogOut[activeTouches * cvPerTouch + 2] = zCv;
		analogOut[activeTouches * cvPerTouch + 3] = gate;
		++activeTouches;
	}

	// write 0 to disabled touches
	for(int n = maxTouches; n > activeTouches; --n)
	{
		// turn the gate off
		int channel = (n - 1) * cvPerTouch + 3;
		analogOut[channel] = 0;
	}

	//log to console
	if(count % 200 == 0){
		for(unsigned int n = 0; n < touches.size(); ++n)
		{
			const TouchKeysTouch& touch = touches[n];
			if(!touch.enabled)
				continue;
			rt_printf("Touch %d: ", n);
			rt_printf("x: %.3f, y: %.3f, z: %.3f, note: %d\n", touch.x, touch.y, touch.z, touch.note); 
		
		}
		//parser.prettyPrint();
	}
	++count;

	for(unsigned int c = 0; c < context->analogOutChannels; ++c)
	{
		if(analogOutSmooth[c])
		{
			for(unsigned int n = 0; n < context->analogFrames; ++n)
			{
				// smooth the analogOut values
				float out = analogOutOld[c] * smooth + analogOut[c] * (1 - smooth);
				analogOutOld[c] = out;
				analogWriteOnce(context, n, c, out);
			}
		}
		else
		{
			analogWrite(context, 0, c, analogOut[c]);
		}
	}
	for(unsigned int n = 0; n < context->audioFrames; n++){
		for(unsigned int c = 0; c < context->audioInChannels; ++c)
			audioWrite(context, n, c, audioRead(context, n, c));
	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().

void cleanup(BelaContext *context, void *userData)
{

}

/**
\example 05-Communication/MIDI/render.cpp

Connecting MIDI devices to Bela!
-------------------------------

This example needs documentation.

*/
