/*
 * Wave.h
 *
 *  Created on: 2014Äê3ÔÂ13ÈÕ
 *      Author: zesheng.dou
 */

#ifndef WAVE_H_
#define WAVE_H_
#include <stdio.h>
#include <vector>

typedef int  int32;

enum WavChunks {
	RiffHeader = 0x46464952,
	WavRiff = 0x54651475,
	Format = 0x020746d66,
	LabeledText = 0x478747C6,
	Instrumentation = 0x478747C6,
	Sample = 0x6C706D73,
	Fact = 0x47361666,
	Data = 0x61746164,
	Junk = 0x4b4e554a,
};

enum WavFormat {
	PulseCodeModulation = 0x01,
	IEEEFloatingPoint = 0x03,
	ALaw = 0x06,
	MuLaw = 0x07,
	IMAADPCM = 0x11,
	YamahaITUG723ADPCM = 0x16,
	GSM610 = 0x31,
	ITUG721ADPCM = 0x40,
	MPEG = 0x50,
	Extensible = 0xFFFE
};

//---------------------------------------------------------------------//
//Definitions in checking wave properties

const int	WAV_VALID=0x0000;	//proper wave file
const int	WAV_ERR1=0x0001;	//AudioFormat!=1
#define	WAV_ERR1_MSG	"audio format != 1"
const int	WAV_ERR2=0x0002;	//NumChannels!=1
#define	WAV_ERR2_MSG	"channel number != 1"
const int	WAV_ERR3=0x0004;	//SampleRate!=48000
#define	WAV_ERR3_MSG	"sample rate != 48kHz"
const int	WAV_ERR4=0x0008;	//BitsPerSample!=16
#define	WAV_ERR4_MSG	"bits per sample != 16-bit"

//---------------------------------------------------------------------//
//Wave file related

struct WAVE_HEADER
{
	WAVE_HEADER() : ChunkID(0), ChunkSize(0), Format(0), SubChunk1ID(0), SubChunk1Size(16), AudioFormat(1), NumChannels(2),
			SampleRate(48000), ByteRate(48000*2*16/8), BlockAligh(2*16/8), BitsPerSample(16), SubChunk2ID(0), SubChunk2Size(0) {}
	//General header
	int32 ChunkID;        //"RIFF"
	int32 ChunkSize;      //Size of the rest of the chunk following this number.
	int32 Format;         //"WAVE"
	//"fmt " subchunk
	int32 SubChunk1ID;    //"fmt "
	int32 SubChunk1Size; //Size of the rest of this subchunk following this number, 16 for PCM.
	short AudioFormat;    //PCM=1
	short NumChannels;    //Mono=1, Stereo=2
	int32 SampleRate;     //8000, 44100, 48000, etc.
	int32 ByteRate;       //SampleRate*NumChannels*BitsPerSample/8
	short BlockAligh;     //NumChannels*BitsPerSample/8
	short BitsPerSample;  //8-bit=8, 16-bit=16
	//"data" subchunk
	int32 SubChunk2ID;    //"data"
	int32 SubChunk2Size;  //NumSamples*NumChannels*BitsPerSample/8
};

struct MY_WAVE_HEADER
{
	WAVE_HEADER WaveHeader;	//wave header
	int32 WaveBeginning;	//address of wave data beginning
};

class Wave
{
public:
	Wave();
	~Wave();

public:
	int ReadWave(const char* filename , std::vector<short>& vecData);

	int GetSampleRate();

private:
	//Load the Mono WaveData to the CWaveBuffer which is a short int buffer
	int ReadWaveFileMono(FILE * fp, std::vector<short>& vecData);

	//Load the Stereo WaveData to the CWaveBuffer which is a short int buffer
	int ReadWaveFileStereo(FILE * fp, std::vector<short>& vecData);

	//Read header information from wave file
	bool ReadWaveHeader(FILE *fp);

	//Check whether this wave file is proper to be detected
	int CheckWaveProperty();

private:
	const char* m_strFileName;
	MY_WAVE_HEADER m_waveHeader;
};

#endif /* WAVE_H_ */
