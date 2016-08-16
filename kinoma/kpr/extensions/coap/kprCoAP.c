/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#include "kprCoAP.h"
#include "kprCoAPServerSession.h"
#include "kprCoAPEndpoint.h"
#include "kprCoAPMessage.h"
#include "kprShell.h"
#include "xs.h"


static KPR_CoAP_RequestHostData KPR_CoAP_request_get(xsMachine *the, xsSlot slot);
static Boolean KPR_CoAP_request_find(xsMachine *the, KPR_CoAP_ClientHostData clientH, KprCoAPMessage request);
static FskErr KPR_CoAP_request_new(xsMachine *the, KPR_CoAP_ClientHostData clientH, KprCoAPMessage request);

static void KPR_CoAP_session_new(xsMachine *the, KprCoAPServerSession session);
static void KPR_CoAP_response_new(xsMachine *the, KprCoAPMessage message);
static KprCoAPMessage KPR_CoAP_response_get(xsMachine *the, xsSlot slot);

static void KPR_CoAP_message_is_confirmable(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_set_confirmable(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_type(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_code(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_method(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_set_method(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_setCode(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_messageId(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_token(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_set_token(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_payload(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_set_payload(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_contentFormat(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_setPayload(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_options(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_addOption(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_is_observe(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_set_observe(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_uri(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_host(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_port(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_path(xsMachine *the, KprCoAPMessage self);
static void KPR_CoAP_message_get_query(xsMachine *the, KprCoAPMessage self);

#define DEFER3(xxx, a, b, c) FskThreadPostCallback(KprShellGetThread(gShell), (FskThreadCallback)xxx, a, b, c, NULL)
#define DEFER2(xxx, a, b) DEFER3(xxx, a, b, NULL)
#define DEFER1(xxx, a) DEFER3(xxx, a, NULL, NULL)

static Boolean gKPR_CoAP_useChunk = false;

static void KPR_CoAP_warnChunkDeprecate(xsMachine *the, const char *domain);

//--------------------------------------------------
// CoAP.Client
//--------------------------------------------------

static FskErr KPR_CoAP_client_responseCallback(KprCoAPMessage request, KprCoAPMessage response, void *refcon);
static FskErr KPR_CoAP_client_acknowledgementCallback(KprCoAPMessage request, void *refcon);
static FskErr KPR_CoAP_client_retryCallback(KprCoAPMessage request, int count, void *refcon);
static FskErr KPR_CoAP_client_deliveryFailureCallback(KprCoAPMessage request, const char *reason, void *refcon);
static FskErr KPR_CoAP_client_requestEndCallback(KprCoAPMessage request, const char *reason, void *refcon);
static void KPR_CoAP_client_errorCallback(FskErr err, const char *reason, void *refcon);

void KPR_CoAP_Client(xsMachine *the)
{
	FskErr err;
	KPR_CoAP_ClientHostData clientH;
	KprCoAPClientCallbacks callbacks = {
		KPR_CoAP_client_responseCallback,
		KPR_CoAP_client_acknowledgementCallback,
		KPR_CoAP_client_retryCallback,
		KPR_CoAP_client_deliveryFailureCallback,
		KPR_CoAP_client_requestEndCallback,
		KPR_CoAP_client_errorCallback,
	};

	ENTER_FUNCTION();
	err = kFskErrNone;
	clientH = NULL;

	bailIfError(KprMemPtrNewClear(sizeof(KPR_CoAP_ClientHostRecord), &clientH));

	bailIfError(KprCoAPClientNew(&clientH->client, &callbacks, clientH));

	clientH->the = the;
	clientH->code = the->code;
	clientH->slot = xsThis;
	xsSetHostData(xsThis, clientH);

bail:
	if (err) {
		KprCoAPClientDispose(clientH->client);
		KprMemPtrDispose(clientH);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_client_destructor(void *it)
{
	ENTER_FUNCTION();
	if (it) {
		KPR_CoAP_ClientHostData clientH = it;
		KPR_CoAP_RequestHostData requestH;

		requestH = clientH->requests;
		while (requestH) {
			xsMachine* the = clientH->the;

			xsForget(requestH->slot);
			requestH->client = NULL;

			requestH = requestH->next;
		}
		KprCoAPClientDispose(clientH->client);
		KprMemPtrDispose(clientH);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_client_createRequest(xsMachine *the)
{
	KPR_CoAP_ClientHostData clientH;
	KprCoAPClient client;
	FskErr err;
	KprCoAPMessage request = NULL;
	char *uri;
	int method;
	int argc;

	ENTER_FUNCTION();
	clientH = xsGetHostData(xsThis);
	client = clientH->client;
	err = kFskErrNone;
	uri = NULL;
	method = kKprCoAPRequestMethodGET;
	argc = xsToInteger(xsArgc);

	uri = xsToString(xsArg(0));

	if (argc > 1) {
		if (xsTypeOf(xsArg(1)) == xsStringType) {
			char *str = xsToString(xsArg(1));
			KprCoAPMethodFromString(str, &method);
		} else {
			method = xsToInteger(xsArg(1));
			if (!method) {
				method = kKprCoAPRequestMethodGET;
			}
		}
	}

	bailIfError(KprCoAPClientCreateRequestMessage(client, uri, method, false, &request));
	bailIfError(KPR_CoAP_request_new(the, clientH, request));

bail:
    KprCoAPMessageDispose(request);
    
	if (err) {
		xsThrowIfFskErr(err);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_client_send(xsMachine *the)
{
	KPR_CoAP_ClientHostData clientH;
	KprCoAPClient client;
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	clientH = xsGetHostData(xsThis);
	client = clientH->client;
	requestH = NULL;

	requestH = KPR_CoAP_request_get(the, xsArg(0));
	xsThrowIfNULL(requestH);

	xsThrowIfFskErr(KprCoAPClientSendRequest(client, requestH->request));

	requestH->client = clientH;
	FskListAppend(&clientH->requests, requestH);
	xsRemember(requestH->slot);
	EXIT_FUNCTION();
}

void KPR_CoAP_client_is_autoToken(xsMachine *the)
{
	KPR_CoAP_ClientHostData clientH;
	KprCoAPClient client;

	ENTER_FUNCTION();
	clientH = xsGetHostData(xsThis);
	client = clientH->client;
	xsResult = xsBoolean(client->autoToken);
	EXIT_FUNCTION();
}
void KPR_CoAP_client_set_autoToken(xsMachine *the)
{
	KPR_CoAP_ClientHostData clientH;
	KprCoAPClient client;

	ENTER_FUNCTION();
	clientH = xsGetHostData(xsThis);
	client = clientH->client;
	client->autoToken = xsToBoolean(xsArg(0));
	EXIT_FUNCTION();
}

static FskErr KPR_CoAP_client_responseCallback(KprCoAPMessage request, KprCoAPMessage response, void *refcon)
{
	FskErr err;
	KPR_CoAP_ClientHostData clientH;

	ENTER_FUNCTION();
	err = kFskErrNone;
	clientH = (KPR_CoAP_ClientHostData) refcon;

	xsBeginHostSandboxCode(clientH->the, clientH->code);
	xsThis = clientH->slot;
	xsVars(2);

	{
		xsTry {
			Boolean hasRequest;
			int onResponse = xsID("onResponse");

			hasRequest = KPR_CoAP_request_find(the, clientH, request);
			xsVar(0) = xsResult;

			KPR_CoAP_response_new(the, response);
			xsVar(1) = xsResult;

			if (hasRequest && xsFindResult(xsVar(0), onResponse)) {
				(void) xsCallFunction1(xsResult, xsVar(0), xsVar(1));
			} else if (xsFindResult(xsThis, onResponse)) {
				(void) xsCallFunction2(xsResult, xsThis, xsVar(0), xsVar(1));
			}
		}
		xsCatch {
			err = kFskErrUnknown;
		}
	}

	xsEndHostSandboxCode();
	RETURN_FUNCTION();
}

static FskErr KPR_CoAP_client_acknowledgementCallback(KprCoAPMessage request, void *refcon)
{
	FskErr err;
	KPR_CoAP_ClientHostData clientH;

	ENTER_FUNCTION();
	err = kFskErrNone;
	clientH = (KPR_CoAP_ClientHostData) refcon;

	xsBeginHostSandboxCode(clientH->the, clientH->code);
	xsThis = clientH->slot;
	xsVars(1);

	{
		xsTry {
			int onAck = xsID("onAck");

			err = KPR_CoAP_request_new(the, clientH, request);
			if (err == kFskErrNone) {
				xsVar(0) = xsResult;

				if (xsFindResult(xsVar(0), onAck)) {
					(void) xsCallFunction0(xsResult, xsVar(0));
				} else if (xsFindResult(xsThis, onAck)) {
					(void) xsCallFunction1(xsResult, xsThis, xsVar(0));
				}
			}
		}
		xsCatch {
		}
	}

	xsEndHostSandboxCode();
	RETURN_FUNCTION();
}

static FskErr KPR_CoAP_client_retryCallback(KprCoAPMessage request, int count, void *refcon)
{
	FskErr err;
	KPR_CoAP_ClientHostData clientH;

	ENTER_FUNCTION();
	err = kFskErrNone;
	clientH = (KPR_CoAP_ClientHostData) refcon;

	xsBeginHostSandboxCode(clientH->the, clientH->code);
	xsThis = clientH->slot;
	xsVars(2);

	{
		xsTry {
			int onRetry = xsID("onRetry");

			err = KPR_CoAP_request_new(the, clientH, request);
			if (err == kFskErrNone) {
				xsVar(0) = xsResult;
				xsVar(1) = xsInteger(count);

				if (xsFindResult(xsVar(0), onRetry)) {
					(void) xsCallFunction1(xsResult, xsVar(0), xsVar(1));
				} else if (xsFindResult(xsThis, onRetry)) {
					(void) xsCallFunction2(xsResult, xsThis, xsVar(0), xsVar(1));
				}
			}
		}
		xsCatch {
		}
	}

	xsEndHostSandboxCode();
	RETURN_FUNCTION();
}

static FskErr KPR_CoAP_client_deliveryFailureCallback(KprCoAPMessage request, const char *reason, void *refcon)
{
	FskErr err;
	KPR_CoAP_ClientHostData clientH;

	ENTER_FUNCTION();
	err = kFskErrNone;
	clientH = (KPR_CoAP_ClientHostData) refcon;

	xsBeginHostSandboxCode(clientH->the, clientH->code);
	xsThis = clientH->slot;
	xsVars(2);

	{
		xsTry {
			int onFailure = xsID("onFailure");

			err = KPR_CoAP_request_new(the, clientH, request);
			if (err == kFskErrNone) {
				xsVar(0) = xsResult;
				xsVar(1) = xsString((char *) reason);

				if (xsFindResult(xsVar(0), onFailure)) {
					(void) xsCallFunction1(xsResult, xsVar(0), xsVar(1));
				} else if (xsFindResult(xsThis, onFailure)) {
					(void) xsCallFunction2(xsResult, xsThis, xsVar(0), xsVar(1));
				}
			}
		}
		xsCatch {
		}
	}

	xsEndHostSandboxCode();
	RETURN_FUNCTION();
}

static FskErr KPR_CoAP_client_requestEndCallback(KprCoAPMessage request, const char *reason, void *refcon)
{
	FskErr err;
	KPR_CoAP_ClientHostData clientH;
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	err = kFskErrNone;
	clientH = (KPR_CoAP_ClientHostData) refcon;

	requestH = clientH->requests;
	while (requestH) {
		if (requestH->request == request) {
			FskListRemove(&clientH->requests, requestH);
			requestH->client = NULL;

			xsBeginHostSandboxCode(clientH->the, clientH->code);

			xsForget(requestH->slot);

			xsEndHostSandboxCode();
			break;
		}
		requestH = requestH->next;
	}

	RETURN_FUNCTION();
}

static void KPR_CoAP_client_errorCallback(FskErr err, const char *reason, void *refcon)
{
	KPR_CoAP_ClientHostData clientH;

	ENTER_FUNCTION();
	clientH = (KPR_CoAP_ClientHostData) refcon;

	xsBeginHostSandboxCode(clientH->the, clientH->code);
	xsThis = clientH->slot;
	xsVars(1);

	{
		xsTry {
			xsVar(0) = xsString((char *) reason);

			if (xsFindResult(xsThis, xsID("onError"))) {
				(void) xsCallFunction1(xsResult, xsThis, xsVar(0));
			}
		}
		xsCatch {

		}
	}

	xsEndHostSandboxCode();
	EXIT_FUNCTION();
}

//--------------------------------------------------
// CoAP.Request
//--------------------------------------------------

#define KPR_CoAP_RequestPrototype xsGet(xsGet(xsGet(xsGlobal, xsID_CoAP), xsID_Request), xsID_prototype)

static KPR_CoAP_RequestHostData KPR_CoAP_request_get(xsMachine *the, xsSlot slot)
{
	ENTER_FUNCTION();
	if (!xsIsInstanceOf(slot, KPR_CoAP_RequestPrototype)) return NULL;
	return xsGetHostData(slot);
	EXIT_FUNCTION();
}

static Boolean KPR_CoAP_request_find(xsMachine *the, KPR_CoAP_ClientHostData clientH, KprCoAPMessage request)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = NULL;

	requestH = clientH->requests;
	while (requestH) {
		if (requestH->request == request) {
			xsResult = requestH->slot;
			return true;
		}
		requestH = requestH->next;
	}

	xsResult = xsNull;
	return false;
	EXIT_FUNCTION();
}

static FskErr KPR_CoAP_request_new(xsMachine *the, KPR_CoAP_ClientHostData clientH, KprCoAPMessage request)
{
	FskErr err;
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	err = kFskErrNone;
	requestH = NULL;

	if (KPR_CoAP_request_find(the, clientH, request)) return kFskErrNone;

	xsResult = xsNew0(xsGet(xsGlobal, xsID("CoAP")), xsID("Request"));

	bailIfError(KprMemPtrNewClear(sizeof(KPR_CoAP_RequestHostRecord), &requestH));

	requestH->request = KprCoAPMessageRetain(request);

	requestH->the = the;
	requestH->code = the->code;
	requestH->slot = xsResult;

	xsSetHostData(xsResult, requestH);

bail:
	if (err) {
		xsResult = xsNull;
		KprMemPtrDispose(requestH);
	}
	RETURN_FUNCTION();
}

void KPR_CoAP_request_destructor(void *it)
{
	ENTER_FUNCTION();
	if (it) {
		KPR_CoAP_RequestHostData requestH = it;

		if (requestH->client) {
			FskListRemove(&requestH->client->requests, requestH);
		}

		KprCoAPMessageDispose(requestH->request);
		KprMemPtrDispose(requestH);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_request_is_confirmable(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_is_confirmable(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_set_confirmable(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_set_confirmable(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_type(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_type(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_messageId(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_messageId(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_method(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_method(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_set_method(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_set_method(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_token(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_token(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_set_token(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_set_token(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_payload(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_payload(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_set_payload(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_set_payload(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_contentFormat(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_contentFormat(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_setPayload(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_setPayload(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_options(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_options(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_addOption(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_addOption(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_is_observe(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_is_observe(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_set_observe(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_set_observe(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_uri(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_uri(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_host(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_host(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_port(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_port(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_path(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_path(the, requestH->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_request_get_query(xsMachine *the)
{
	KPR_CoAP_RequestHostData requestH;

	ENTER_FUNCTION();
	requestH = xsGetHostData(xsThis);
	KPR_CoAP_message_get_query(the, requestH->request);
	EXIT_FUNCTION();
}

//--------------------------------------------------
// CoAP.Server
//--------------------------------------------------

static FskErr KPR_CoAP_server_resourceCallback(KprCoAPServerSession session, void *refcon);
static void KPR_CoAP_server_retryCallback(KprCoAPMessage message, UInt32 retryCount, void *refcon);
static FskErr KPR_CoAP_server_handleResource(xsMachine *the, char *path);
static FskErr KPR_CoAP_server_wellKnownCore(xsMachine *the);
static void KPR_CoAP_server_errorCallback(FskErr err, const char *reason, void *refcon);

void KPR_CoAP_Server(xsMachine *the)
{
	FskErr err;
	KPR_CoAP_ServerHostData serverH;
	KprCoAPServerCallbacks callbacks = {
		KPR_CoAP_server_resourceCallback,
		KPR_CoAP_server_retryCallback,
		KPR_CoAP_server_errorCallback,
	};

	ENTER_FUNCTION();
	err = kFskErrNone;
	serverH = NULL;

	xsVars(1);

	bailIfError(KprMemPtrNewClear(sizeof(KPR_CoAP_ServerHostRecord), &serverH));

	bailIfError(KprCoAPServerNew(&callbacks, serverH, &serverH->server));

	serverH->the = the;
	serverH->code = the->code;
	serverH->slot = xsThis;
	xsSetHostData(xsThis, serverH);

	xsVar(0) = xsNewInstanceOf(xsArrayPrototype);
	xsNewHostProperty(xsThis, xsID("resources"), xsVar(0), xsDefault, xsDontScript);

bail:
	if (err) {
		KprCoAPServerDispose(serverH->server);
		KprMemPtrDispose(serverH);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_server_destructor(void *it)
{
	ENTER_FUNCTION();
	if (it) {
		KPR_CoAP_ServerHostData serverH = it;

		KprCoAPServerDispose(serverH->server);
		KprMemPtrDispose(serverH);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_server_get_port(xsMachine *the)
{
	KPR_CoAP_ServerHostData serverH;
	KprCoAPServer server;

	ENTER_FUNCTION();
	serverH = xsGetHostData(xsThis);
	server = serverH->server;
	xsResult = xsInteger(server->port);
	EXIT_FUNCTION();
}

void KPR_CoAP_server_start(xsMachine *the)
{
	KPR_CoAP_ServerHostData serverH;
	KprCoAPServer server;
	FskErr err;
	UInt16 port;
	int argc;
	char *interfaceName;

	ENTER_FUNCTION();
	serverH = xsGetHostData(xsThis);
	server = serverH->server;
	err = kFskErrNone;
	port = 0;
	argc = xsToInteger(xsArgc);
	interfaceName = NULL;

	if (argc > 0) {
		port = xsToInteger(xsArg(0));
	}
	if (!port) port = 5683;

	if (argc > 1 && xsTest(xsArg(1))) {
		interfaceName = xsToString(xsArg(1));
	}

	bailIfError(KprCoAPServerStart(server, port, interfaceName));

bail:
	if (err) {
		xsThrowIfFskErr(err);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_server_stop(xsMachine *the)
{
	KPR_CoAP_ServerHostData serverH;
	KprCoAPServer server;
	FskErr err;

	ENTER_FUNCTION();
	serverH = xsGetHostData(xsThis);
	server = serverH->server;
	err = kFskErrNone;

	bailIfError(KprCoAPServerStop(server));

bail:
	if (err) {
		xsThrowIfFskErr(err);
	}
	EXIT_FUNCTION();
}

#define DEBUG_DUMP(x) xsCall1(xsGet(xsGlobal, xsID("DEBUG")), xsID("dump"), x);

void KPR_CoAP_server_bind(xsMachine *the)
{
	FskErr err;
	int argc;

	ENTER_FUNCTION();
	err = kFskErrNone;
	argc = xsToInteger(xsArgc);

	if (argc < 2) xsThrowIfFskErr(kFskErrInvalidParameter);

	xsVars(1);

	if (argc > 2) {
		if (xsIsInstanceOf(xsArg(2), xsObjectPrototype)) {
			xsVar(0) = xsArg(2);
		} else {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsVar(0), xsID_description, xsArg(2), xsDefault, xsDontScript);
		}
	} else {
		xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
		xsNewHostProperty(xsVar(0), xsID_description, xsNull, xsDefault, xsDontScript);
	}
	xsNewHostProperty(xsVar(0), xsID_path, xsArg(0), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID_callback, xsArg(1), xsDefault, xsDontScript);

//	DEBUG_DUMP(xsString("sever.bind()"));
//	DEBUG_DUMP(xsArg(0));
//	DEBUG_DUMP(xsVar(0));
	(void)xsCall1(xsGet(xsThis, xsID("resources")), xsID_push, xsVar(0));

	xsThrowIfFskErr(err);
	EXIT_FUNCTION();
}

void KPR_CoAP_server_getSession(xsMachine *the)
{
	KPR_CoAP_ServerHostData serverH;
	KprCoAPServer server;
	KprCoAPServerSession session;
	FskErr err;
	UInt32 sessId;

	ENTER_FUNCTION();
	serverH = xsGetHostData(xsThis);
	server = serverH->server;
	err = kFskErrNone;
	sessId = xsToInteger(xsArg(0));

	err = KprCoAPServerGetRunningSession(server, sessId, &session);
	if (err == kFskErrNone) {
		KPR_CoAP_session_new(the, session);
	} else {
		xsResult = xsNull;
	}
	EXIT_FUNCTION();
}

static FskErr KPR_CoAP_server_resourceCallback(KprCoAPServerSession session, void *refcon)
{
	FskErr err;
	KPR_CoAP_ServerHostData serverH;
	char *path;

	ENTER_FUNCTION();
	err = kFskErrNone;
	serverH = (KPR_CoAP_ServerHostData) refcon;
	path = (char *) session->request->path;

	xsBeginHostSandboxCode(serverH->the, serverH->code);
	xsThis = serverH->slot;

	xsVars(3);
	{
		xsTry {
			KPR_CoAP_session_new(the, session);
			xsVar(0) = xsResult;

			if (FskStrCompare("xx/.well-known/core", path) == 0) {
				err = KPR_CoAP_server_wellKnownCore(the);
			} else {
				err = KPR_CoAP_server_handleResource(the, path);
			}
		}
		xsCatch {
			err = kFskErrUnknown;
		}
	}

	xsEndHostSandboxCode();
	RETURN_FUNCTION();
}

static void KPR_CoAP_server_retryCallback(KprCoAPMessage message, UInt32 retryCount, void *refcon)
{
	KPR_CoAP_ServerHostData serverH;

	ENTER_FUNCTION();
	serverH = (KPR_CoAP_ServerHostData) refcon;

	xsBeginHostSandboxCode(serverH->the, serverH->code);
	xsThis = serverH->slot;
	xsVars(1);

	{
		xsTry {
			//		xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
			//		xsNewHostProperty(xsVar(0), xsID("code"), xsInteger(err), xsDefault, xsDontScript);
			//		xsNewHostProperty(xsVar(0), xsID("reason"), xsString((char *) reason), xsDefault, xsDontScript);
			//
			//		if (xsFindResult(xsThis, xsID("onError"))) {
			//			(void) xsCallFunction1(xsResult, xsThis, xsVar(0));
			//		}
		}
		xsCatch {

		}
	}

	xsEndHostSandboxCode();
	EXIT_FUNCTION();
}

static FskErr KPR_CoAP_server_handleResource(xsMachine *the, char *path)
{
	FskErr err;
	int len, i;

	ENTER_FUNCTION();
	xsVar(1) = xsGet(xsThis, xsID("resources"));
	len = xsToInteger(xsGet(xsVar(1), xsID_length));
	err = kFskErrNotFound;

	for (i = 0; i < len; i++) {
		char *path2;

		xsVar(2) = xsGetAt(xsVar(1), xsInteger(i));
		path2 = xsToString(xsGet(xsVar(2), xsID_path));

		if (FskStrCompare(path, path2) == 0 || FskStrCompare(path, "*") == 0) {
			(void) xsCallFunction1(xsGet(xsVar(2), xsID_callback), xsVar(2), xsVar(0));
			err = kFskErrNone;
			break;
		}
	}
	RETURN_FUNCTION();
}

static FskErr KPR_CoAP_server_wellKnownCore(xsMachine *the)
{
	FskErr err;
	int len, i;

	ENTER_FUNCTION();
	xsVar(1) = xsGet(xsThis, xsID("resources"));
	len = xsToInteger(xsGet(xsVar(1), xsID_length));
	err = kFskErrNotFound;

	for (i = 0; i < len; i++) {
		char *path2;

		xsVar(2) = xsGetAt(xsVar(1), xsInteger(i));
		path2 = xsToString(xsGet(xsVar(2), xsID_path));

		(void) xsCallFunction1(xsGet(xsVar(2), xsID_callback), xsVar(2), xsVar(0));
	}
	RETURN_FUNCTION();
}

static void KPR_CoAP_server_errorCallback(FskErr err, const char *reason, void *refcon)
{
	KPR_CoAP_ServerHostData serverH;

	ENTER_FUNCTION();
	serverH = (KPR_CoAP_ServerHostData) refcon;

	xsBeginHostSandboxCode(serverH->the, serverH->code);
	xsThis = serverH->slot;
	xsVars(1);

	{
		xsTry {
			xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
			xsNewHostProperty(xsVar(0), xsID("code"), xsInteger(err), xsDefault, xsDontScript);
			xsNewHostProperty(xsVar(0), xsID("reason"), xsString((char *) reason), xsDefault, xsDontScript);

			if (xsFindResult(xsThis, xsID("onError"))) {
				(void) xsCallFunction1(xsResult, xsThis, xsVar(0));
			}
		}
		xsCatch {

		}
	}

	xsEndHostSandboxCode();
	EXIT_FUNCTION();
}

//--------------------------------------------------
// CoAP.Session
//--------------------------------------------------

static void KPR_CoAP_session_new(xsMachine *the, KprCoAPServerSession session)
{
	ENTER_FUNCTION();
	xsResult = xsNew0(xsGet(xsGlobal, xsID("CoAP")), xsID("Session"));
	xsSetHostData(xsResult, KprCoAPServerSessionRetain(session));
	EXIT_FUNCTION();
}

void KPR_CoAP_session_destructor(void *it)
{
	ENTER_FUNCTION();
	if (it) {
		KprCoAPServerSession self = (KprCoAPServerSession) it;
		KprCoAPServerSessionDispose(self);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_id(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->sessionId);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_is_confirmable(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_is_confirmable(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_type(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_type(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_messageId(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_messageId(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_method(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_method(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_token(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_token(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_payload(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_payload(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_contentFormat(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_contentFormat(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_options(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_options(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_remoteIP(xsMachine *the)
{
	KprCoAPServerSession self;
	char remoteIP[64];

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	FskNetIPandPortToString(self->endpoint->ipaddr, 0, remoteIP);
	xsResult = xsString(remoteIP);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_remotePort(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	xsResult = xsInteger(self->endpoint->port);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_uri(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_uri(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_host(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_host(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_port(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_port(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_path(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_path(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_get_query(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_query(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_createResponse(xsMachine *the)
{
	KprCoAPServerSession self;
	KprCoAPMessage response;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);

	xsThrowIfFskErr(KprCoAPServerSessionCreateResponse(self, &response));
	KPR_CoAP_response_new(the, response);
	KprCoAPMessageDispose(response);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_send(xsMachine *the)
{
	KprCoAPServerSession self;
	KprCoAPMessage response;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);

	response = KPR_CoAP_response_get(the, xsArg(0));
	xsThrowIfNULL(response);

	xsThrowIfFskErr(KprCoAPServerSessionSendResponse(self, response));
	EXIT_FUNCTION();
}

void KPR_CoAP_session_is_observe(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_is_observe(the, self->request);
	EXIT_FUNCTION();
}

void KPR_CoAP_session_acceptObserve(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);

	xsThrowIfFskErr(KprCoAPServerSessionAcceptObserve(self));
	EXIT_FUNCTION();
}

void KPR_CoAP_session_endObserve(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);

	xsThrowIfFskErr(KprCoAPServerSessionEndObserve(self));
	EXIT_FUNCTION();
}

void KPR_CoAP_server_is_autoAck(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	xsResult = xsBoolean(self->autoAck);
	EXIT_FUNCTION();
}

void KPR_CoAP_server_set_autoAck(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	self->autoAck = xsToBoolean(xsArg(0));
	EXIT_FUNCTION();
}

void KPR_CoAP_session_sendAck(xsMachine *the)
{
	KprCoAPServerSession self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);

	xsThrowIfFskErr(KprCoAPServerSessionSendAck(self));
	EXIT_FUNCTION();
}

//--------------------------------------------------
// CoAP.Response
//--------------------------------------------------

#define KPR_CoAP_ResponsePrototype xsGet(xsGet(xsGet(xsGlobal, xsID_CoAP), xsID_Response), xsID_prototype)

static void KPR_CoAP_response_new(xsMachine *the, KprCoAPMessage message)
{
	ENTER_FUNCTION();
	xsResult = xsNew0(xsGet(xsGlobal, xsID("CoAP")), xsID("Response"));
	xsSetHostData(xsResult, KprCoAPMessageRetain(message));
	EXIT_FUNCTION();
}

static KprCoAPMessage KPR_CoAP_response_get(xsMachine *the, xsSlot slot)
{
	ENTER_FUNCTION();
	if (!xsIsInstanceOf(slot, KPR_CoAP_ResponsePrototype)) return NULL;
	EXIT_FUNCTION();
	return xsGetHostData(slot);
}

void KPR_CoAP_response_destructor(void *it)
{
	ENTER_FUNCTION();
	if (it) {
		KprCoAPMessage self = (KprCoAPMessage) it;
		KprCoAPMessageDispose(self);
	}
	EXIT_FUNCTION();
}

void KPR_CoAP_response_is_confirmable(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_is_confirmable(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_set_confirmable(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_set_confirmable(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_type(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_type(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_messageId(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_messageId(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_code(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_code(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_setCode(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_setCode(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_token(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_token(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_payload(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_payload(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_set_payload(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_set_payload(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_contentFormat(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_contentFormat(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_setPayload(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_setPayload(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_options(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_options(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_addOption(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_addOption(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_is_observe(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_is_observe(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_uri(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_uri(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_host(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_host(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_port(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_port(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_path(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_path(the, self);
	EXIT_FUNCTION();
}

void KPR_CoAP_response_get_query(xsMachine *the)
{
	KprCoAPMessage self;

	ENTER_FUNCTION();
	self = xsGetHostData(xsThis);
	KPR_CoAP_message_get_query(the, self);
	EXIT_FUNCTION();
}

static KprCoAPContentFormat KPR_CoAP_getFormatAndContents(xsMachine *the, const void **ptr, UInt32 *length)
{
	if (xsTypeOf(xsArg(0)) == xsStringType) {
		*ptr = xsToString(xsArg(0));
		*length = FskStrLen(*ptr);
		return kKprCoAPContentFormatPlainText;
	} else if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
		KPR_CoAP_warnChunkDeprecate(the, "request.payload");

		*ptr = xsGetHostData(xsArg(0));
		*length = xsToInteger(xsGet(xsArg(0), xsID_length));
		return kKprCoAPContentFormatOctetStream;
	} else {
		*ptr = xsToArrayBuffer(xsArg(0));
		*length = xsGetArrayBufferLength(xsArg(0));
		return kKprCoAPContentFormatOctetStream;
	}
}

static void KPR_CoAP_message_is_confirmable(xsMachine *the, KprCoAPMessage self)
{
	xsResult = xsBoolean(self->type == kKprCoAPMessageTypeConfirmable);
}

static void KPR_CoAP_message_set_confirmable(xsMachine *the, KprCoAPMessage self)
{
	Boolean confirmable = xsToBoolean(xsArg(0));

	if (self->frozen) xsThrowIfFskErr(kFskErrBadState);

	self->type = (confirmable ? kKprCoAPMessageTypeConfirmable : kKprCoAPMessageTypeNonConfirmable);
}

static void KPR_CoAP_message_get_type(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	const char *str;

	err = KprCoAPMessageTypeToString(self->type, &str);
	xsThrowIfFskErr(err);
	xsResult = xsString((char *) str);
}

static void KPR_CoAP_message_get_code(xsMachine *the, KprCoAPMessage self)
{
	int code = self->code;
	int cls = (code >> 5) & 0x7;
	int detail = (code) & 0x1f;
	char value[5];

	sprintf(value, "%d.%02d", cls, detail);

	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsResult, xsID("class"), xsInteger(cls), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("detail"), xsInteger(detail), xsDefault, xsDontScript);
	xsNewHostProperty(xsResult, xsID("value"), xsString(value), xsDefault, xsDontScript);
}

static void KPR_CoAP_message_get_method(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	const char *str;
	int method = (self->code) & 0x1f;

	err = KprCoAPMethodToString(method, &str);
	if (err == kFskErrNone) {
		xsResult = xsString((char *) str);
	}
}
static void KPR_CoAP_message_set_method(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	int method;

	if (self->frozen) xsThrowIfFskErr(kFskErrBadState);

	if (xsTypeOf(xsArg(0)) == xsStringType) {
		err = KprCoAPMethodFromString(xsToString(xsArg(0)), &method);
		xsThrowIfFskErr(err);
	} else {
		method = xsToInteger(xsArg(0));
	}

	self->code = KprCoAPMessageCodeWith(0, method);
}

static void KPR_CoAP_message_setCode(xsMachine *the, KprCoAPMessage self)
{
	int argc = xsToInteger(xsArgc), cls, detail;

	if (self->frozen) xsThrowIfFskErr(kFskErrBadState);

	if (argc != 2) xsThrowIfFskErr(kFskErrInvalidParameter);

	cls = xsToInteger(xsArg(0));
	detail = xsToInteger(xsArg(1));

	self->code = KprCoAPMessageCodeWith(cls, detail);
}

static void KPR_CoAP_message_get_messageId(xsMachine *the, KprCoAPMessage self)
{
	xsResult = xsInteger(self->messageId);
}

static void KPR_CoAP_message_get_token(xsMachine *the, KprCoAPMessage self)
{
	if (self->token) {
		if (gKPR_CoAP_useChunk) {
			xsThrowIfFskErr(KprMemoryBlockToChunk(self->token, the, &xsResult));
		} else {
			xsResult = xsArrayBuffer(KprMemoryBlockStart(self->token), self->token->size);
		}
	} else {
		xsResult = xsNull;
	}
}

static void KPR_CoAP_message_set_token(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	UInt32 len;
	void *data;

	if (self->frozen) xsThrowIfFskErr(kFskErrBadState);

	if (xsTypeOf(xsArg(0)) == xsStringType) {
		char *token = xsToString(xsArg(0));
		len = FskStrLen(token);
		data = token;
	} else if (xsIsInstanceOf(xsArg(0), xsChunkPrototype)) {
		if (!gKPR_CoAP_useChunk) KPR_CoAP_warnChunkDeprecate(the, "request.token");

		data = xsGetHostData(xsArg(0));
		len = xsToInteger(xsGet(xsArg(0), xsID_length));
	} else {
		data = xsToArrayBuffer(xsArg(0));
		len = xsGetArrayBufferLength(xsArg(0));
	}

	err = KprCoAPMessageSetToken(self, (const void *) data, len);
	xsThrowIfFskErr(err);
}

static void KPR_CoAP_message_get_payload(xsMachine *the, KprCoAPMessage self)
{
	if (self->payload) {
		KprCoAPContentFormat format = KprCoAPMessageGetContentFormat(self);

		if (format == kKprCoAPContentFormatNone) format = kKprCoAPContentFormatPlainText;

		switch (format) {
			case kKprCoAPContentFormatPlainText:
			case kKprCoAPContentFormatNone:
			case kKprCoAPContentFormatJson:
			case kKprCoAPContentFormatXml:
			case kKprCoAPContentFormatLinkFormat:
				xsResult = xsString((char *) KprMemoryBlockStart(self->payload));
				break;

			default:
				if (gKPR_CoAP_useChunk) {
					xsThrowIfFskErr(KprMemoryBlockToChunk(self->payload, the, &xsResult));
				} else {
					xsResult = xsArrayBuffer(KprMemoryBlockStart(self->payload), self->payload->size);
				}
				break;
		}
	} else {
		xsResult = xsNull;
	}
}

static void KPR_CoAP_message_set_payload(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	const void *payload = NULL;
	UInt32 length = 0;
	KprCoAPContentFormat format;

	if (self->frozen) xsThrowIfFskErr(kFskErrBadState);

	format = KPR_CoAP_getFormatAndContents(the, &payload, &length);

	err = KprCoAPMessageSetPayload(self, payload, length);
	xsThrowIfFskErr(err);

	err = KprCoAPMessageSetContentFormat(self, format);
	xsThrowIfFskErr(err);
}

static void KPR_CoAP_message_get_contentFormat(xsMachine *the, KprCoAPMessage self)
{
	KprCoAPContentFormat format = KprCoAPMessageGetContentFormat(self);

	xsResult = xsUndefined;

	if (format != kKprCoAPContentFormatNone) {
		FskErr err;
		const char *str;

		err = KprCoAPContentFormatToString(format, &str);
		xsThrowIfFskErr(err);
		xsResult = xsString((char *) str);
	}
}

static void KPR_CoAP_message_setPayload(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	const void *payload = NULL;
	UInt32 length = 0;
	KprCoAPContentFormat format;

	if (self->frozen) xsThrowIfFskErr(kFskErrBadState);

	format = KPR_CoAP_getFormatAndContents(the, &payload, &length);

	err = KprCoAPMessageSetPayload(self, payload, length);
	xsThrowIfFskErr(err);

	if (xsTest(xsArg(1))) {
		char *str = xsToString(xsArg(1));

		err = KprCoAPContentFormatFromString(str, &format);
		xsThrowIfFskErr(err);
	}

	err = KprCoAPMessageSetContentFormat(self, format);
	xsThrowIfFskErr(err);
}

static void KPR_CoAP_message_get_options(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	KprCoAPMessageOptionRecord *optRec = self->options;

	xsVars(2);

	xsResult = xsNewInstanceOf(xsArrayPrototype);

	while (optRec) {
		const char *optionStr;

		err = KprCoAPMessageOptionToString(optRec->option, &optionStr);
		xsThrowIfFskErr(err);

		xsVar(0) = xsNewInstanceOf(xsArrayPrototype);

		xsCall1_noResult(xsVar(0), xsID_push, xsString((char *) optionStr));

		switch (optRec->format) {
			case kKprCoAPMessageOptionFormatEmpty:
				xsVar(1) = xsNull;
				break;

			case kKprCoAPMessageOptionFormatOpaque:
				if (gKPR_CoAP_useChunk) {
					// @TODO maybe it is daingerous because if message was
					// disposed before option value, it will crash.
					xsVar(1) = xsNewInstanceOf(xsChunkPrototype);
					xsSetHostData(xsVar(1), (void *) optRec->value.opaque.data);
					xsSetHostDestructor(xsVar(1) , NULL);
					xsSet(xsVar(1), xsID_length, xsInteger(optRec->value.opaque.length));
				} else {
					xsVar(1) = xsArrayBuffer((void*) optRec->value.opaque.data, optRec->value.opaque.length);
				}
				break;

			case kKprCoAPMessageOptionFormatUint:
				xsVar(1) = xsInteger(optRec->value.uint);
				break;

			case kKprCoAPMessageOptionFormatString:
				xsVar(1) = xsString((char *) optRec->value.string);
				break;

		}

		xsCall1_noResult(xsVar(0), xsID_push, xsVar(1));

		xsCall1_noResult(xsResult, xsID_push, xsVar(0));

		optRec = optRec->next;
	}
}

static void KPR_CoAP_message_addOption(xsMachine *the, KprCoAPMessage self)
{
	FskErr err;
	int option = 0;
	KprCoAPMessageOptionFormat format;
	xsType argType;
	UInt32 uint;
	char *str;
	void *opaque;
	UInt32 length;

	if (xsTest(xsArg(0))) {
		if (xsTypeOf(xsArg(0)) == xsStringType) {
			char *str = xsToString(xsArg(0));
			KprCoAPMessageOptionFromString(str, &option);
		} else {
			option = xsToInteger(xsArg(0));
		}
	}

	if (!option) {
		xsThrowIfFskErr(kFskErrParameterError);
	}

	format = KprCoAPMessageOptionGetFormat(option);
	if (xsToInteger(xsArgc) > 1) {
		argType = xsTypeOf(xsArg(1));
	} else {
		argType = xsUndefinedType;
	}

	switch (format) {
		case kKprCoAPMessageOptionFormatEmpty:
			if (argType != xsUndefinedType && argType != xsNullType) {
				err = kFskErrInvalidParameter;
			}
			else {
				err = KprCoAPMessageAppendEmptyOption(self, option);
			}
			break;

		case kKprCoAPMessageOptionFormatOpaque:
			if (xsTypeOf(xsArg(1)) == xsStringType) {
				opaque = xsToString(xsArg(1));
				length = FskStrLen(opaque);
			} else if (xsIsInstanceOf(xsArg(1), xsChunkPrototype)) {
				if (!gKPR_CoAP_useChunk) KPR_CoAP_warnChunkDeprecate(the, "message.addOption opaque");

				opaque = xsGetHostData(xsArg(1));
				length = xsToInteger(xsGet(xsArg(1), xsID_length));
			} else {
				opaque = xsToArrayBuffer(xsArg(1));
				length = xsGetArrayBufferLength(xsArg(1));
			}
			err = KprCoAPMessageAppendOpaqueOption(self, option, opaque, length);
			break;

		case kKprCoAPMessageOptionFormatUint:
			uint = xsToInteger(xsArg(1));
			err = KprCoAPMessageAppendUintOption(self, option, uint);
			break;

		case kKprCoAPMessageOptionFormatString:
			str = xsToString(xsArg(1));
			err = KprCoAPMessageAppendStringOption(self, option, str);
			break;

		default:
			err = kFskErrUnimplemented;
			break;
	}
	xsThrowIfFskErr(err);
}

static void KPR_CoAP_message_is_observe(xsMachine *the, KprCoAPMessage self)
{
	KprCoAPMessageOptionRecord *optRec;
	optRec = KprCoAPMessageFindOption(self, kKprCoAPMessageOptionObserve);
	xsResult = xsBoolean(optRec != NULL);
}

static void KPR_CoAP_message_set_observe(xsMachine *the, KprCoAPMessage self)
{
	FskErr err = kFskErrNone;
	KprCoAPMessageOptionRecord *optRec;
	optRec = KprCoAPMessageFindOption(self, kKprCoAPMessageOptionObserve);

	if (xsTest(xsArg(0))) {
		if (optRec == NULL) {
			err = KprCoAPMessageAppendUintOption(self, kKprCoAPMessageOptionObserve, kKprCoAPMessageObserveRegister);
		}
	} else {
		if (optRec != NULL) {
			err = KprCoAPMessageRemoveOptions(self, kKprCoAPMessageOptionObserve);
		}
	}
}

static void KPR_CoAP_message_get_uri(xsMachine *the, KprCoAPMessage self)
{
	if (self->uri) {
		xsResult = xsString((char *) self->uri);
	} else {
		xsResult = xsNull;
	}
}

static void KPR_CoAP_message_get_host(xsMachine *the, KprCoAPMessage self)
{
	if (self->host) {
		xsResult = xsString((char *) self->host);
	} else {
		xsResult = xsNull;
	}
}

static void KPR_CoAP_message_get_port(xsMachine *the, KprCoAPMessage self)
{
	if (self->port) {
		xsResult = xsInteger(self->port);
	} else {
		xsResult = xsNull;
	}
}

static void KPR_CoAP_message_get_path(xsMachine *the, KprCoAPMessage self)
{
	if (self->path) {
		xsResult = xsString((char *) self->path);
	} else {
		xsResult = xsNull;
	}
}

static void KPR_CoAP_message_get_query(xsMachine *the, KprCoAPMessage self)
{
	if (self->query) {
		xsResult = xsString((char *) self->query);
	} else {
		xsResult = xsNull;
	}
}

void KPR_CoAP_get_useChunk(xsMachine *the)
{
	ENTER_FUNCTION();
	xsResult = xsBoolean(gKPR_CoAP_useChunk);
	EXIT_FUNCTION();
}

void KPR_CoAP_set_useChunk(xsMachine *the)
{
	Boolean flag;
	ENTER_FUNCTION();
	flag = xsToBoolean(xsArg(0));
	if (!gKPR_CoAP_useChunk && flag) {
		KPR_CoAP_warnChunkDeprecate(the, "useChunk");
	}
	gKPR_CoAP_useChunk = flag;
	EXIT_FUNCTION();
}

static void KPR_CoAP_warnChunkDeprecate(xsMachine *the, const char *domain)
{
	char buf[256];
	sprintf(buf, "WARNING: %s: Chunk is deprecated. Will be dropped off soon. Use ArrayBuffer insted.\n", domain);
	xsTrace(buf);
}

//--------------------------------------------------
// CoAP Function Test
//--------------------------------------------------

#if defined(RUN_UNITTEST) && RUN_UNITTEST

#include "kunit.h"

ku_main();
ku_test(CoAP_message);

void KPR_CoAP_test()
{
	ku_begin();
	ku_run(CoAP_message);
	ku_finish();
}

#endif

//--------------------------------------------------
// CoAP Library Interface
//--------------------------------------------------


FskErr kprCoAP_fskLoad(FskLibrary library)
{
#if defined(RUN_UNITTEST) && RUN_UNITTEST
	KPR_CoAP_test();
#endif
	return kFskErrNone;
}

FskErr kprCoAP_fskUnload(FskLibrary library)
{
	return kFskErrNone;
}

static Boolean KPR_CoAP_defineEntry(int index, int value, const char *symbol, void *refcon1, void *refcon2)
{
	xsMachine *the = (xsMachine *) refcon1;

	int sym_id = xsID((char *) symbol);
	xsNewHostProperty(xsResult, sym_id, xsInteger(value), xsDontDelete | xsDontSet, xsDontScript | xsDontDelete | xsDontSet);
	return false;
}

void KPR_CoAP_patch(xsMachine* the)
{
	static KprCoAPStringMapEntry sOptions[] = {
		{ 1, "IfMatch" },
		{ 3, "UriHost" },
		{ 4, "ETag" },
		{ 5, "IfNoneMatch" },
		{ 7, "UriPort" },
		{ 8, "LocationPath" },
		{ 11, "UriPath" },
		{ 12, "ContentFormat" },
		{ 14, "MaxAge" },
		{ 15, "UriQuery" },
		{ 17, "Accept" },
		{ 20, "LocationQuery" },
		{ 35, "ProxyUri" },
		{ 39, "ProxyScheme" },
		{ 60, "Size1" },
	};
	static KprCoAPStringMapEntry sMethods[] = {
		{ 1, "GET" },
		{ 2, "POST" },
		{ 3, "PUT" },
		{ 4, "DELETE" },
	};

	{
		KprCoAPStringMapDefine(map, sOptions);
		xsResult = xsGet(xsGet(xsGlobal, xsID("CoAP")), xsID("Option"));
		KprCoAPStringMapForEach(&map, KPR_CoAP_defineEntry, the, NULL);
	}

	{
		KprCoAPStringMapDefine(map, sMethods);
		xsResult = xsGet(xsGet(xsGlobal, xsID("CoAP")), xsID("Method"));
		KprCoAPStringMapForEach(&map, KPR_CoAP_defineEntry, the, NULL);
	}

	{
		KprCoAPStringMapDefine(map, sMethods);
		xsResult = xsGet(xsGlobal, xsID("CoAP"));
		KprCoAPStringMapForEach(&map, KPR_CoAP_defineEntry, the, NULL);
	}
}


