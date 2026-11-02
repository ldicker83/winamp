#ifndef NULLSOFT_XSPF_API_H
#define NULLSOFT_XSPF_API_H

// Service Manager
#include "api/service/api_service.h"

// Media Library API

#include "../Agave/Language/api_language.h"

#include "../tagz/api_tagz.h"
extern api_tagz *tagzApi;
#define AGAVE_API_TAGZ tagzApi

#include <api/application/api_application.h>
#define WASABI_API_APP applicationApi

#endif