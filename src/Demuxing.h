/*
 * Demuxing.h
 * Created on: 2014-03-13
 *     Author: zesheng.dou
 */

#ifndef DEMUXING_H_
#define DEMUXING_H_
//#define __STDC_FORMAT_MACROS
//#define __STDC_CONSTANT_MACROS
extern "C"
{
//#define snprintf _snprintf
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(55, 45, 101)
#define AV_FRAME_ALLOC av_frame_alloc
#define AV_GET_FRAME_DEFAULTS av_frame_unref
#else
#define AV_FRAME_ALLOC avcodec_alloc_frame
#define AV_GET_FRAME_DEFAULTS avcodec_get_frame_defaults
#endif

#define USEAUDIO
//#define USEVIDEO

const int SAMPLERATE=48000;
class Demuxing
{
public:
	Demuxing(bool bWithVideo = true, bool bWithAudio = true);
	~Demuxing();

	bool InitVideo(const char* strFileName);

	bool GetDataFromVideo(char** pData,int& iDataLen, int& iType);

	bool CloseVideo();

	float GetFPS(){return m_fFPS;};
private:
	int decode_packet(int *got_frame,char** pData,int& iDataLen, int& iType);

	int open_input_file(const char *filename);

#ifdef USEVIDEO
	int init_video_stream( const char *filters_descr, AVPixelFormat fmt, AVFilterGraph **video_filter_graph, 
		AVFilterContext **video_buffersrc_ctx, AVFilterContext **video_buffersink_ctx);
	int decode_video_packet(int *got_frame,char** pData,int& iDataLen);
#endif

#ifdef USEAUDIO
	int decode_audio_packet(int *got_frame,char** pData,int& iDataLen);
	int init_audio_stream(const char *filters_descr);
#endif

private:
	AVFormatContext *m_fmt_ctx;
	AVPacket m_pkt;

	AVFrame *m_frame;
	AVFrame *m_filt_frame;

#ifdef USEVIDEO
	// video [3/22/2014 zesheng.dou]
	int m_video_stream_idx;
	AVCodecContext *m_video_dec_ctx;
	int m_video_frame_count;

	char m_video_filter_desc[1024];
	AVFilterContext *m_video_buffersink_ctx;
	AVFilterContext *m_video_buffersrc_ctx;
	AVFilterGraph *m_video_filter_graph;

	//for video YUV mode
	AVFilterContext *m_videoYUV_buffersrc_ctx;
	AVFilterContext *m_videoYUV_buffersink_ctx;
	AVFilterGraph *m_videoYUV_filter_graph;
#endif

#ifdef USEAUDIO
	// audio [3/22/2014 zesheng.dou]
	int m_audio_stream_idx;
	AVCodecContext * m_audio_dec_ctx;
	int m_audio_frame_count;

	char m_audio_filter_desc[1024];
	AVFilterContext *m_audio_buffersink_ctx;
	AVFilterContext *m_audio_buffersrc_ctx;
	AVFilterGraph *m_audio_filter_graph;
#endif

	// other [3/25/2014 zesheng.dou]
	bool m_bReadNewPkt;
public:
	float m_fFPS;
};
#endif
