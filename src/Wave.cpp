/*
* Wave.cpp
*
*  Created on: 2014-03-13
*      Author: zesheng.dou
*/

#include "Wave.h"

Wave::Wave()
{
	// TODO Auto-generated constructor stub
	m_strFileName = NULL;
	m_waveHeader.WaveBeginning = 0;
}

Wave::~Wave()
{
	// TODO Auto-generated destructor stub
}

bool Wave::ReadWaveHeader(FILE* fp)
{
    size_t result;
    int fseekRtn = -1;
	//open wave file
	if ( !fp )
	{
		return false;
	}
	rewind(fp);
	int32 chunkid = 0;
	bool datachunk = false;
	while ( (!datachunk) ) 
	{
		fread(&chunkid, 4, 1, fp);
		// if read the end of the file, return.
		if (feof(fp))
		{
			break;
		}
		switch ( (WavChunks)chunkid ) 
		{
		case Format:
			m_waveHeader.WaveHeader.SubChunk1ID = chunkid;
			//Reading subchunk1, totally 24 Bytes
			result = fread(&(m_waveHeader.WaveHeader.SubChunk1Size), 20, 1, fp);

			if ( m_waveHeader.WaveHeader.SubChunk1Size != 16 ) {
				short sExtSkip = 0;
				result = fread(&sExtSkip, 2, 1, fp);
				fseekRtn = fseek(fp, sExtSkip, SEEK_CUR);
			}
			break;
		case RiffHeader:
			m_waveHeader.WaveHeader.ChunkID = chunkid;
			result = fread(&(m_waveHeader.WaveHeader.ChunkSize), 8, 1, fp);
			break;
		case Data:
			m_waveHeader.WaveHeader.SubChunk2ID = chunkid;
			result = fread(&(m_waveHeader.WaveHeader.SubChunk2Size), 4, 1, fp);
			datachunk = true;
			m_waveHeader.WaveBeginning = ftell(fp);
			break;
		default:
			int32 skipsize = 0;
			result = fread(&skipsize, 4, 1, fp);
			//Then, Reading subchunk2, totally 4 Bytes
			fseekRtn = fseek(fp, skipsize, SEEK_CUR);
			break;
		}
	}
	return true;
}

int Wave::CheckWaveProperty()
{
	int result = WAV_VALID;

	if ( m_waveHeader.WaveHeader.AudioFormat != 1 )
	{
		result |= WAV_ERR1;
	}
	if ( m_waveHeader.WaveHeader.NumChannels != 1 )
	{
		result |= WAV_ERR2;
	}
	if ( m_waveHeader.WaveHeader.SampleRate != 48000 )
	{
		result |= WAV_ERR3;
	}
	if ( m_waveHeader.WaveHeader.BitsPerSample != 16 )
	{
		result |= WAV_ERR4;
	}

	return result;
}

int Wave::ReadWaveFileMono(FILE* fp, std::vector<short>& vecData)
{
    int fseekRtn = -1;
    size_t result;
	short wavedata;

	if ( !fp )
	{
		return 1;
	}

	rewind(fp);

	int datalen = m_waveHeader.WaveHeader.SubChunk2Size / sizeof(short);

	fseekRtn = fseek(fp, (m_waveHeader.WaveBeginning), SEEK_SET);

	vecData.clear();

	for (int i = 0; i < datalen; i++)
	{
		result = fread(&wavedata, sizeof(short), 1, fp);
		vecData.push_back(wavedata);
	}

	return 0;
}

int Wave::ReadWaveFileStereo(FILE* fp, std::vector<short>& vecData, bool bStereoAsMono)
{
	short wavedatal; //Left Channel
	short wavedatar; //Right Channel
	int fseekRtn = -1;

	if ( !fp )
	{
		return 1;
	}

	rewind(fp);

	int datalen = m_waveHeader.WaveHeader.SubChunk2Size / (sizeof(short) * 2);

	fseekRtn = fseek(fp, (m_waveHeader.WaveBeginning), SEEK_SET);

	vecData.clear();

	for (int i = 0; i < datalen; i++)
	{
		size_t lSize = fread(&wavedatal, sizeof(short), 1, fp);
		size_t rSize = fread(&wavedatar, sizeof(short), 1, fp);
		if(bStereoAsMono)
		{
			if ( (wavedatal + wavedatar) >= 0 )
			{
				vecData.push_back((short) (0.5 * (wavedatal + wavedatar) + 0.5));
			}
			else
			{
				vecData.push_back((short) (0.5 * (wavedatal + wavedatar) - 0.5));
			}
		}
		else
		{
			vecData.push_back(lSize);
			vecData.push_back(rSize);
		}
	}

	return 0;
}

int Wave::ReadWave(const char* filename, std::vector<short>& vecData, bool bStereoAsMono)
{
	m_strFileName = filename;

	FILE* fp=fopen(m_strFileName,"rb");

	if(!fp)
	{
		return 1;
	}

	if(!ReadWaveHeader(fp))
	{
		fclose(fp);
		return 2;
	}

	if(m_waveHeader.WaveHeader.BitsPerSample == 16)
	{
		if(m_waveHeader.WaveHeader.NumChannels==1){

			ReadWaveFileMono(fp,vecData);

		}else if(m_waveHeader.WaveHeader.NumChannels==2){

			ReadWaveFileStereo(fp,vecData, bStereoAsMono);

		}else{
			fclose(fp);
			return 3;
		}
	}
	fclose(fp);
	return 0;
}

int Wave::GetSampleRate()
{
	return m_waveHeader.WaveHeader.SampleRate;
}
