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

#include "UrlRequest.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace analytics {
namespace utils {

UrlRequest::UrlRequest(const std::string& host, const std::string& uri, Options options) :
	// params
	mHost(host),

	// defaults
	mCallback(nullptr),
	mBytesRead(0),
	mContentLength(0)
{

	// create request and set header
	mHttpRequest = HttpRequest(methodToString(options.method), uri, options.version);
	mHttpRequest.setHeader("Host", mHost);

	// set body and automatically set content length
	if (options.body) {
		mHttpRequest.setHeader("Content-Length", toString<size_t>(options.body->getSize()));
		mHttpRequest.setBody(options.body);
	}

	// set additional user headers (overwrites any defaults)
	for (const KeyValuePair& kvp : options.headers) {
		mHttpRequest.setHeader(kvp.first, kvp.second);
	}

	mClient = TcpClient::create(AppBase::get()->io_service());
}

UrlRequest::~UrlRequest() {
	mCallback = nullptr; // bb: delete callback because at this point we can't refer to `this` anymore
	close();
}

void UrlRequest::connect(Callback callback, int port) {
	if (isConnected()) {
		cout << "UrlRequest: Warning: Session already open; Aborting" << endl;
		return;
	}

	std::lock_guard<std::recursive_mutex> lock(mMutex);
	
	//cout << "UrlRequest: connecting" << endl;

	mCallback = callback;

	mHttpResponse = HttpResponse();

	mBytesRead = 0;
	mContentLength = 0;

	mClient->connectConnectEventHandler(&UrlRequest::onConnect, this);
	mClient->connectErrorEventHandler(&UrlRequest::onError, this);
	mClient->connectResolveEventHandler(&UrlRequest::onResolve, this);

	mClient->connect(mHost, port);
}

void UrlRequest::close() {
	std::lock_guard<std::recursive_mutex> lock(mMutex);
	
	//cout << "UrlRequest: closing" << endl;

	try {
		if (mSession) {
			mSession->connectCloseEventHandler(nullptr);
			mSession->connectErrorEventHandler(nullptr);
			mSession->connectReadEventHandler(nullptr);
			mSession->connectWriteEventHandler(nullptr);
			mSession->close();
			mSession = nullptr;
		}

		if (mClient) {
			mClient->connectConnectEventHandler(nullptr);
			mClient->connectErrorEventHandler(nullptr);
			mClient->connectResolveEventHandler(nullptr);
			mClient->getResolver()->cancel();
			mClient = nullptr;
		}

		if (mCallback) {
			mCallback(shared_from_this());
			mCallback = nullptr;
		}

	} catch (Exception e) {
		cout << "UrlRequest: Error on close: " << e.what() << endl;
	}

}

//==================================================
// Accessors
// 

bool UrlRequest::isConnected() {
	std::lock_guard<std::recursive_mutex> lock(mMutex);
	return mSession && mSession->getSocket()->is_open();
}

bool UrlRequest::isCompleted() {
	if (isConnected()) return false;
	std::lock_guard<std::recursive_mutex> lock(mMutex);
	return mBytesRead >= mContentLength;
}

bool UrlRequest::wasSuccessful() {
	if (!isCompleted()) return false;
	switch (mHttpResponse.getStatusCode()) {
		case 200:
		case 201:
			return true;
		default:
			return false;
	}
}

ci::BufferRef UrlRequest::getBody() {
	if (!isCompleted()) return nullptr;
	return mHttpResponse.getBody();
}

std::string UrlRequest::getBodyText() {
	try {
		if (auto body = getBody()) {
			return HttpResponse::bufferToString(body);
		}
	} catch (Exception e) {
		cout << "UrlRequest: Could not get body text: " << e.what() << endl;
	}

	return "";
}

//==================================================
// Callbacks
// 

void UrlRequest::onConnect(TcpSessionRef session) {
	std::lock_guard<std::recursive_mutex> lock(mMutex);

	//cout << "UrlRequest: connected" << endl;

	mSession = session;

	mSession->connectCloseEventHandler(&UrlRequest::onClose, this);
	mSession->connectErrorEventHandler(&UrlRequest::onError, this);
	mSession->connectReadEventHandler(&UrlRequest::onRead, this);
	mSession->connectWriteEventHandler(&UrlRequest::onWrite, this);

	mSession->write(mHttpRequest.toBuffer());
}

void UrlRequest::onError(string err, size_t bytesTransferred) {
	std::lock_guard<std::recursive_mutex> lock(mMutex);
	//cout << "UrlRequest: error : '" << err << "'" << endl;
	close();
}

void UrlRequest::onResolve() {
	//cout << "UrlRequest: resolved" << endl;
}

void UrlRequest::onWrite(size_t bytesTransferred) {
	//cout << "UrlRequest: writing " << to_string(bytesTransferred) << " bytes to buffer" << endl;
	mSession->read();
}

void UrlRequest::onRead(ci::BufferRef buffer) {
	std::lock_guard<std::recursive_mutex> lock(mMutex);
	//cout << "UrlRequest: reading from buffer" << endl;

	if (!mHttpResponse.hasHeader()) {
		// Parse header
		mHttpResponse.parseHeader(HttpResponse::bufferToString(buffer));
		buffer = HttpResponse::removeHeader(buffer);

		// Get content-length
		for (const KeyValuePair& kvp : mHttpResponse.getHeaders()) {
			if (kvp.first == "Content-Length") {
				mContentLength = fromString<size_t>(kvp.second);
				break;
			}
		}
	}

	// Append buffer to body
	mBytesRead += buffer->getSize();
	mHttpResponse.append(buffer);

	if (mBytesRead < mContentLength) {
		// Keep reading until we hit the content length
		mSession->read();

	} else {
		// Close session when done
		close();
	}
}

void UrlRequest::onClose() {
	std::lock_guard<std::recursive_mutex> lock(mMutex);
	//cout << "UrlRequest: closed" << endl;
	mSession = nullptr;
}

//==================================================
// Helpers
// 

std::string UrlRequest::methodToString(Method method) {
	switch (method) {
		case GET: return "GET";
		case POST: return "POST";
		case PUT: return "PUT";
		case DEL: return "DELETE";
		case PATCH: return "PATCH";
	}
	return "";
}

} // utils namespace
} // analytics namespace
} // bluecadet namespace
