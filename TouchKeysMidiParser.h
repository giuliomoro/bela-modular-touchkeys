#include <Midi.h>
#include <vector>

class TouchKeysTouch
{
public: 
	float x;
	unsigned int yMsb;
	unsigned int yLsb;
	float y;
	float z;
	midi_byte_t note;
	bool enabled;

	TouchKeysTouch() :
		x(0)
		, yMsb(0)
		, yLsb(0)
		, y(0)
		, z(0)
		, note(0)
		, enabled(false)
	{}

	void computeY()
	{
		y = (yMsb * 128 + yLsb) / (float)(128 * 128);
	}
};


class TouchKeysMidiParser
{
public:
	TouchKeysMidiParser()
	{
		touches.resize(16);
	}

	static void midiMessageCallback(MidiChannelMessage message, void* arg)
	{
		TouchKeysMidiParser* that = (TouchKeysMidiParser*)arg;
		//rt_printf("Message from TouchKeys port %s ", (const char*) arg);
		//message.prettyPrint();
		midi_byte_t channel = message.getChannel();
		MidiMessageType type = message.getType();
		TouchKeysTouch& touch = that->touches[channel];

		if(type == kmmNoteOn || type == kmmNoteOff)
		{
			midi_byte_t note = message.getDataByte(0);
			midi_byte_t velocity = message.getDataByte(1);

			if(type == kmmNoteOff)
				velocity = 0;

			if(velocity)
			{
				touch.enabled = true;
				touch.note = note;
			}
			else
				touch.enabled = false;
		}

		if(type == kmmControlChange)
		{
			midi_byte_t control = message.getDataByte(0);
			midi_byte_t value = message.getDataByte(1);
			switch (control)
			{
				case ccX:
					touch.x = value / 128.f;
				break;
				case ccYMsb:
					touch.yMsb = value;
				break;
				case ccYLsb:
					touch.yLsb = value;
					touch.computeY();
				break;
				case ccZ:
					touch.z = value / 128.f;
				break;
			}
		}
	}

	void prettyPrint()
	{
		for(unsigned int n = 0; n < touches.size(); ++n)
		{
			TouchKeysTouch& touch = touches[n];
			if(!touch.enabled)
				continue;
			rt_printf("Touch %d: ", n);
			rt_printf("x: %.3f, y: %.3f, z: %.3f, note: %d\n", touch.x, touch.y, touch.z, touch.note); 

		}
	}

	 const std::vector<TouchKeysTouch>& getTouches()
	 {
	 	return touches;	
	 }

private:
	std::vector<TouchKeysTouch> touches;
	static const midi_byte_t ccX = 20;
	static const midi_byte_t ccYMsb = 21;
	static const midi_byte_t ccYLsb = 53;
	static const midi_byte_t ccZ = 22;
};
