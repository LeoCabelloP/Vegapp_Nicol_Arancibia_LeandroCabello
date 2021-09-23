/*
* DSD Converter plugin
* Copyright (c) 2016-2019 Maxim V.Anisiutkin <maxim.anisiutkin@gmail.com>
*/

#ifndef _DSD_PROCESSOR_SERVICE_H_INCLUDED
#define _DSD_PROCESSOR_SERVICE_H_INCLUDED

#include <foobar2000.h>

class dsd_processor_service : public service_base {
public:
	virtual GUID get_guid() = 0;
	virtual const char* get_name() = 0;
	virtual bool is_active() = 0;
	virtual bool is_changed() = 0;
	virtual void set_volume(double p_volume_dB) = 0;
	virtual bool start(t_size p_inp_channels, unsigned p_inp_samplerate, unsigned p_inp_channel_config, t_size& p_out_channels, unsigned& p_out_samplerate, unsigned& p_out_channel_config) = 0;
	virtual void stop() = 0;
	virtual const t_uint8* run(const void* p_inp_pcmdsd, t_size p_inp_samples, t_size* p_out_samples) = 0;
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(dsd_processor_service);
};

#endif
