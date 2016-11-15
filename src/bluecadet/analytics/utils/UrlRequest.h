/*
 BSD 3-Clause License
 
 Copyright (c) 2016, Bluecadet
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 
 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.
 
 * Neither the name of the copyright holder nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/Json.h"

#include "CinderAsio.h"
#include "TcpClient.h"
#include "bantherewind/protocol/HttpRequest.h"
#include "bantherewind/protocol/HttpResponse.h"

namespace bluecadet {
namespace analytics {
namespace utils {

typedef std::shared_ptr<class UrlRequest> UrlRequestRef;

class UrlRequest : public std::enable_shared_from_this<UrlRequest> {

public:
	//==================================================
	// Helper Types
	// 
	typedef std::function<void(UrlRequestRef request)> Callback;
	
	enum Method { PUT, POST, GET, PATCH, DEL /*can't use reserved keyword DELETE, so use DEL*/ };

	struct Options {
		Method			method = GET;		//! HTTP method
		ci::BufferRef	body = nullptr;		//! Optional body to send with the request; Automatically sets CONTENT-LENGTH header.
		HeaderMap		headers;			//! Overrides all default header values
		HttpVersion		version = HttpVersion::HTTP_1_1;

		//! Convenience method to set body from string
		void setBodyText(const std::string & bodyStr) { body = HttpRequest::stringToBuffer(bodyStr); }
	};

public:
	//==================================================
	// Public
	// 
	static UrlRequestRef create(const std::string & host, const std::string & uri = "/", Options options = Options()) {
		return UrlRequestRef(new UrlRequest(host, uri, options));
	}

	UrlRequest(const std::string & host, const std::string & uri = "/", Options options = Options());
	~UrlRequest();

	void						connect(Callback callback = nullptr, int port = 80);
	void						close();

	bool						isConnected();
	bool						isCompleted();
	bool						wasSuccessful();

	ci::BufferRef				getBody();
	std::string					getBodyText();

	const HttpRequest&			getHttpRequest() { return mHttpRequest; };
	const HttpResponse&			getHttpResponse() { return mHttpResponse; };

private:
	//==================================================
	// Private
	// 

	std::recursive_mutex		mMutex;
	Callback					mCallback;

	TcpClientRef				mClient;
	TcpSessionRef				mSession;
	std::string					mHost;

	size_t						mBytesRead;
	size_t						mContentLength;
	HttpRequest					mHttpRequest;
	HttpResponse				mHttpResponse;

	//==================================================
	// Callbacks
	// 
	void						onClose();
	void						onConnect(TcpSessionRef session);
	void						onError(std::string err, size_t bytesTransferred);
	void						onRead(ci::BufferRef buffer);
	void						onResolve();
	void						onWrite(size_t bytesTransferred);

	//==================================================
	// Helpers
	// 
	std::string					methodToString(Method method);
};

} // utils namespace
} // analytics namespace
} // bluecadet namespace
