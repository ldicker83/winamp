#include "api__mp3-mpg123.h"

#include "../nu/Singleton.h"
#include "../nu/factoryt.h"
#include "adts_mpg123.h"
#include <bfc/platform/export.h>
#include "../Agave/Component/ifc_wa5component.h"
#include "../nu/ServiceBuilder.h"
#include <mpg123.h>

api_service *WASABI_API_SVC    = 0;
api_config  *AGAVE_API_CONFIG  = 0;
api_memmgr  *WASABI_API_MEMMGR = 0;

class MP3DecoderComponent : public ifc_wa5component
{
public:
	void RegisterServices(api_service *service);
	int RegisterServicesSafeModeOk();
	void DeregisterServices(api_service *service);

protected:
	RECVS_DISPATCH;
};

static ServiceFactoryT<adts, ADTS_MPG123> adts_factory;


void MP3DecoderComponent::RegisterServices(api_service *service)
{
	WASABI_API_SVC = service;
	if (mpg123_init() == MPG123_OK) {
		adts_factory.Register(WASABI_API_SVC);
	}
	ServiceBuild(WASABI_API_SVC, AGAVE_API_CONFIG, AgaveConfigGUID);
	ServiceBuild(WASABI_API_SVC, WASABI_API_MEMMGR, memMgrApiServiceGuid);
}

void MP3DecoderComponent::DeregisterServices(api_service *service)
{
	adts_factory.Deregister(WASABI_API_SVC);
	ServiceRelease(WASABI_API_SVC, AGAVE_API_CONFIG, AgaveConfigGUID);
	ServiceRelease(WASABI_API_SVC, WASABI_API_MEMMGR, memMgrApiServiceGuid);
	mpg123_exit();
}

int MP3DecoderComponent::RegisterServicesSafeModeOk()
{
	return 1;
}

MP3DecoderComponent mp3_decoder_component;
extern "C" DLLEXPORT ifc_wa5component *GetWinamp5SystemComponent()
{
	return &mp3_decoder_component;
}

#define CBCLASS MP3DecoderComponent
START_DISPATCH;
VCB(API_WA5COMPONENT_REGISTERSERVICES, RegisterServices)
CB(15, RegisterServicesSafeModeOk)
VCB(API_WA5COMPONENT_DEREEGISTERSERVICES, DeregisterServices)
END_DISPATCH;
#undef CBCLASS