#include "Demuxing.h"

extern "C"
{
#define snprintf _snprintf
#include <libavutil/opt.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

Demuxing::Demuxing(bool bUseVideo, bool bUseAudio)
{
	/* register all formats and codecs */
	avcodec_register_all();
	av_register_all();
	avfilter_register_all();

	m_fmt_ctx = NULL;
	m_frame =  NULL;
	m_filt_frame = NULL;

#ifdef USEVIDEO
	m_video_frame_count=0;
	m_video_dec_ctx = NULL;
	m_video_filter_graph = NULL;
	m_videoYUV_filter_graph = NULL;
	sprintf(m_video_filter_desc,"scale=640:480");
#endif

#ifdef USEAUDIO
	m_audio_frame_count=0;
	m_audio_dec_ctx = NULL;
	m_audio_filter_graph = NULL;
	sprintf(m_audio_filter_desc,"aformat=sample_fmts=s16:sample_rates=%d:channel_layouts=mono",SAMPLERATE);
	//sprintf(m_audio_filter_desc,"aformat=sample_fmts=s16:sample_rates=%d:channel_layouts=stereo",SAMPLERATE);
#endif
}

Demuxing::~Demuxing()
{
	CloseVideo();
}

bool Demuxing::InitVideo( const char* strFileName )
{
	int ret;
	if ((ret = open_input_file(strFileName)) < 0)
	{
		CloseVideo();
		return false;
	}
#ifdef USEVIDEO
	if ((ret = init_video_stream(m_video_filter_desc,AV_PIX_FMT_BGR24,&m_video_filter_graph,&m_video_buffersrc_ctx,&m_video_buffersink_ctx)) < 0)
	{
		CloseVideo();
		return false;
	}
	if ((ret = init_video_stream(m_video_filter_desc,AV_PIX_FMT_YUVJ444P,&m_videoYUV_filter_graph,&m_videoYUV_buffersrc_ctx,&m_videoYUV_buffersink_ctx)) < 0)
	{
		CloseVideo();
		return false;
	}
#endif

#ifdef USEAUDIO
	if ((ret = init_audio_stream(m_audio_filter_desc)) < 0)
	{
		CloseVideo();
		return false;
	}
#endif

	/* dump input information to stderr */
	av_dump_format(m_fmt_ctx, 0, strFileName, 0);
	
	m_frame = AV_FRAME_ALLOC();
	m_filt_frame = AV_FRAME_ALLOC();
	if (!m_frame|| !m_filt_frame) {
		fprintf(stderr, "Could not allocate frame\n");
		ret = AVERROR(ENOMEM);
		CloseVideo();
		return false;
	}

	/* initialize packet, set data to NULL, let the demuxer fill it */
	av_init_packet(&m_pkt);
	m_pkt.data = NULL;
	m_pkt.size = 0;

	m_bReadNewPkt = true;

	/* read frames from the file */
	/*while (av_read_frame(m_fmt_ctx, &m_pkt) >= 0) {
		AVPacket orig_pkt = m_pkt;
		do {
			ret = decode_packet(&got_frame);
			if (ret < 0)
				break;
			m_pkt.data += ret;
			m_pkt.size -= ret;
		} while (m_pkt.size > 0);
		av_free_packet(&orig_pkt);
	}//*/
	return true;
}

bool Demuxing::CloseVideo()
{
	if(m_fmt_ctx)
	{
		avformat_close_input(&m_fmt_ctx);
		m_fmt_ctx = NULL;
	}
	if (m_frame)
	{
		av_free(m_frame);
		m_frame =  NULL;
	}
	if (m_filt_frame)
	{
		av_free(m_filt_frame);
		m_filt_frame = NULL;
	}
#ifdef USEVIDEO
	if (m_video_dec_ctx)
	{
		avcodec_close(m_video_dec_ctx);
		m_video_dec_ctx = NULL;
	}
	if (m_video_filter_graph)
	{
		avfilter_graph_free(&m_video_filter_graph);
		m_video_filter_graph = NULL;
	}
	if (m_videoYUV_filter_graph)
	{
		avfilter_graph_free(&m_videoYUV_filter_graph);
		m_videoYUV_filter_graph = NULL;
	}
#endif

#ifdef USEAUDIO
	if (m_audio_dec_ctx)
	{
		avcodec_close(m_audio_dec_ctx);
		m_audio_dec_ctx = NULL;
	}
	if (m_audio_filter_graph)
	{
		avfilter_graph_free(&m_audio_filter_graph);
		m_audio_filter_graph = NULL;
	}
#endif
	return true;
}

int Demuxing::decode_packet( int *got_frame, char** pData,int& iDataLen, int& iType)
{	
	int ret = 0;
	int decoded = m_pkt.size;
	char strRessult[1024];

	iType = 0;
	AV_GET_FRAME_DEFAULTS(m_frame);
#ifdef USEVIDEO
	if (m_pkt.stream_index == m_video_stream_idx) 
	{
		decode_video_packet(got_frame,pData,iDataLen);
		if ( *got_frame )
		{
			iType = 1;
		}		
	}
#endif

#ifdef USEAUDIO
	if (m_pkt.stream_index == m_audio_stream_idx)
	{
		decoded = decode_audio_packet(got_frame,pData, iDataLen);
		if ( *got_frame )
		{
			iType = 2;
		}
	}
#endif

	return decoded;
}

int Demuxing::open_input_file( const char *filename )
{
	int ret;
	AVCodec *dec;

	/* open input file, and allocate format context */
	if ((ret = avformat_open_input(&m_fmt_ctx, filename, NULL, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
		return ret;
	}

	/* retrieve stream information */
	if ((ret = avformat_find_stream_info(m_fmt_ctx, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		return ret;
	}

#ifdef USEVIDEO
	/* select the video stream */
	ret = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &dec, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find a video stream in the input file\n");
		return ret;
	}
	m_video_stream_idx = ret;
	m_video_dec_ctx = m_fmt_ctx->streams[m_video_stream_idx]->codec;

	/* init the video decoder */
	if ((ret = avcodec_open2(m_video_dec_ctx, dec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open video decoder\n");
		return ret;
	}
#endif

#ifdef USEAUDIO
	/* select the audio stream */
	ret = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot find a audio stream in the input file\n");
		return ret;
	}
	m_audio_stream_idx = ret;
	m_audio_dec_ctx = m_fmt_ctx->streams[m_audio_stream_idx]->codec;
	av_opt_set_int(m_audio_dec_ctx, "refcounted_frames", 1, 0);

	/* init the audio decoder */
	if ((ret = avcodec_open2(m_audio_dec_ctx, dec, NULL)) < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
		return ret;
	}
#endif
	/* Get video fps here */
	//m_fFPS = m_video_dec_ctx->time_base.den*0.5f/m_video_dec_ctx->time_base.num;

	return 0;
}

#ifdef USEVIDEO
int Demuxing::init_video_stream( const char *filters_descr, AVPixelFormat fmt, AVFilterGraph **video_filter_graph, 
	AVFilterContext **video_buffersrc_ctx, AVFilterContext **video_buffersink_ctx)
{
	char args[512];
	int ret;
	AVFilter *buffersrc  = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("buffersink");
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();
	enum AVPixelFormat pix_fmts[] = { fmt, AV_PIX_FMT_NONE };

	*video_filter_graph = avfilter_graph_alloc();

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	sprintf(args,
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		m_video_dec_ctx->width, m_video_dec_ctx->height, m_video_dec_ctx->pix_fmt,
		m_video_dec_ctx->time_base.num, m_video_dec_ctx->time_base.den,
		m_video_dec_ctx->sample_aspect_ratio.num, m_video_dec_ctx->sample_aspect_ratio.den);

	ret = avfilter_graph_create_filter(&*video_buffersrc_ctx, buffersrc, "in",
		args, NULL, *video_filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		return ret;
	}

	/* buffer video sink: to terminate the filter chain. */
	if ((ret = avfilter_graph_create_filter(video_buffersink_ctx, buffersink, "out",
		NULL, NULL, *video_filter_graph))<0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		return ret;
	}
	if ((ret = av_opt_set_int_list(*video_buffersink_ctx, "pix_fmts", pix_fmts,  AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink format\n");
		return ret;
	}

	/* Endpoints for the filter graph. */
	outputs->name       = av_strdup("in");
	outputs->filter_ctx = *video_buffersrc_ctx;
	outputs->pad_idx    = 0;
	outputs->next       = NULL;

	inputs->name       = av_strdup("out");
	inputs->filter_ctx = *video_buffersink_ctx;
	inputs->pad_idx    = 0;
	inputs->next       = NULL;

	if ((ret = avfilter_graph_parse_ptr(*video_filter_graph, filters_descr,
		&inputs, &outputs, NULL)) < 0)
		return ret;
	
	if ((ret = avfilter_graph_config(*video_filter_graph, NULL)) < 0)
		return ret;

	avfilter_inout_free(&outputs);
	avfilter_inout_free(&inputs);
	return 0;
}

int Demuxing::decode_video_packet( int *got_frame, char** pData,int& iDataLen)
{
	/* decode video frame */
	int ret = avcodec_decode_video2(m_video_dec_ctx, m_frame, got_frame, &m_pkt);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Error decoding video\n");
		return ret;
	}

	if (*got_frame)
	{
		m_frame->pts = av_frame_get_best_effort_timestamp(m_frame);

		/* push the decoded frame into the filtergraph */
		if (av_buffersrc_add_frame_flags(m_video_buffersrc_ctx, m_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
			return 0;
		}
		/* push the decoded frame into the filtergraph */
		if (av_buffersrc_add_frame_flags(m_videoYUV_buffersrc_ctx, m_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error while feeding the YUV filtergraph\n");
			return 0;
		}

		/* pull filtered frames from the filtergraph */
		while (1) {
			ret = av_buffersink_get_frame(m_video_buffersink_ctx, m_filt_frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			if (ret < 0)
				break;
			iDataLen = m_filt_frame->width*m_filt_frame->height*6;
			*pData = new char[iDataLen];
			memcpy(*pData,m_filt_frame->data[0],iDataLen/2);
			av_frame_unref(m_filt_frame);
			ret = av_buffersink_get_frame(m_videoYUV_buffersink_ctx, m_filt_frame);
			memcpy(*pData+iDataLen/2,m_filt_frame->data[0],m_filt_frame->width*m_filt_frame->height);
			memcpy(*pData+iDataLen*2/3,m_filt_frame->data[1],m_filt_frame->width*m_filt_frame->height);
			memcpy(*pData+iDataLen - m_filt_frame->width*m_filt_frame->height,m_filt_frame->data[2],m_filt_frame->width*m_filt_frame->height);
			av_frame_unref(m_filt_frame);
		}
	}
	return 0;
}
#endif

#ifdef USEAUDIO
int Demuxing::init_audio_stream( const char *filters_descr )
{	
	char args[512];
	int ret;
	AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
	AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs  = avfilter_inout_alloc();
	static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
	static const int64_t out_channel_layouts[] = { AV_CH_LAYOUT_MONO, -1 };
	static const int out_sample_rates[] = { SAMPLERATE, -1 };
	const AVFilterLink *outlink;
	AVRational time_base = m_fmt_ctx->streams[m_audio_stream_idx]->time_base;

	m_audio_filter_graph = avfilter_graph_alloc();

	/* buffer audio source: the decoded frames from the decoder will be inserted here. */
	if (!m_audio_dec_ctx->channel_layout)
		m_audio_dec_ctx->channel_layout = av_get_default_channel_layout(m_audio_dec_ctx->channels);
	sprintf(args,
		"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%llx",
		time_base.num, time_base.den, m_audio_dec_ctx->sample_rate,
		av_get_sample_fmt_name(m_audio_dec_ctx->sample_fmt), m_audio_dec_ctx->channel_layout);
	ret = avfilter_graph_create_filter(&m_audio_buffersrc_ctx, abuffersrc, "in",
		args, NULL, m_audio_filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
		return ret;
	}

	/* buffer audio sink: to terminate the filter chain. */
	ret = avfilter_graph_create_filter(&m_audio_buffersink_ctx, abuffersink, "out",
		NULL, NULL, m_audio_filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
		return ret;
	}

	/* Endpoints for the filter graph. */
	outputs->name       = av_strdup("in");
	outputs->filter_ctx = m_audio_buffersrc_ctx;
	outputs->pad_idx    = 0;
	outputs->next       = NULL;

	inputs->name       = av_strdup("out");
	inputs->filter_ctx = m_audio_buffersink_ctx;
	inputs->pad_idx    = 0;
	inputs->next       = NULL;

	if ((ret = avfilter_graph_parse_ptr(m_audio_filter_graph, filters_descr,
		&inputs, &outputs, NULL)) < 0)
		return ret;

	if ((ret = avfilter_graph_config(m_audio_filter_graph, NULL)) < 0)
		return ret;

	/* Print summary of the sink buffer
	* Note: args buffer is reused to store channel layout string */
	outlink = m_audio_buffersink_ctx->inputs[0];
	av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
	av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
		(int)outlink->sample_rate,
		(char *)av_x_if_null(av_get_sample_fmt_name((AVSampleFormat)outlink->format), "?"),
		args);

	avfilter_inout_free(&outputs);
	avfilter_inout_free(&inputs);
	return 0;
}

int Demuxing::decode_audio_packet( int *got_frame, char** pData,int& iDataLen )
{
	/* decode audio frame */
	int ret = avcodec_decode_audio4(m_audio_dec_ctx, m_frame, got_frame, &m_pkt);
	if (ret < 0) 
	{
		fprintf(stderr, "Error decoding audio frame\n");
		return ret;
	}
	/* Some audio decoders decode only part of the packet, and have to be
	* called again with the remainder of the packet data.
	* Sample: fate-suite/lossless-audio/luckynight-partial.shn
	* Also, some decoders might over-read the packet. */
	int decoded = FFMIN(ret, m_pkt.size);

	if (*got_frame) 
	{
		/* push the audio data from decoded frame into the filtergraph */
		if (av_buffersrc_add_frame_flags(m_audio_buffersrc_ctx, m_frame, 0) < 0) {
			av_log(NULL, AV_LOG_ERROR, "Error while feeding the audio filtergraph\n");
			return 0;
		}

		/* pull filtered audio from the filtergraph */
		while (1) {
			ret = av_buffersink_get_frame(m_audio_buffersink_ctx, m_filt_frame);
			if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			if(ret < 0)
				break;
			// the count of the samples.
			int n = m_filt_frame->nb_samples* 
				av_get_bytes_per_sample((AVSampleFormat)m_filt_frame->format) * 
				av_get_channel_layout_nb_channels(av_frame_get_channel_layout(m_filt_frame));			
			iDataLen = n;
			*pData = new char[iDataLen];
			memcpy(*pData,m_filt_frame->data[0],iDataLen);
			av_frame_unref(m_filt_frame);
		}
	}
	return decoded;
}
#endif

bool Demuxing::GetDataFromVideo( char** pData,int& iDataLen, int& iType )
{
	if ( m_bReadNewPkt )
	{
		if(av_read_frame(m_fmt_ctx, &m_pkt) < 0)
		{
			return false;
		}
	}
	m_bReadNewPkt = false;
	int got_frame;

	int ret = decode_packet(&got_frame, pData,iDataLen,iType);
	if (ret < 0)
	{
		av_free_packet(&m_pkt);
		m_bReadNewPkt = true;
		return true;
	}

	m_pkt.data += ret;
	m_pkt.size -= ret;
	
	if ( m_pkt.size==0 )
	{
		av_free_packet(&m_pkt);
		m_bReadNewPkt = true;
	}
	return true;
}
