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
#include "cinder/Url.h"

namespace bluecadet {
namespace analytics {
typedef std::shared_ptr<class GAHit> GAHitRef;

class GAHit {
public:

	//! See https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters#sc
	enum SessionControl {
		None,
		Start,
		End
	};

	GAHit(const std::string & appName, const std::string & trackingId, const std::string & clientId, const std::string & version, const std::string & type, const std::string & customQuery = "") :
		mTimestamp(ci::app::getElapsedSeconds()),
		mAppName(appName),
		mTrackingId(trackingId),
		mClientId(clientId),
		mVersion(version),
		mType(type),
		mCustomQuery(customQuery)
	{}
	virtual ~GAHit() {};

	virtual std::string getPayloadString() const {
		// Delta time since hit in miliseconds
		int queueTime = (int)round((ci::app::getElapsedSeconds() - mTimestamp) * 1000.0);

		// Mandatory values
		std::string result =
			"v=" + mVersion +						// API version
			"&an=" + ci::Url::encode(mAppName) +	// App Name
			"&tid=" + mTrackingId +					// Tracking ID
			"&cid=" + mClientId +					// Client ID
			"&t=" + mType +							// Hit type
			"&qt=" + std::to_string(queueTime);		// Queue time 

		// Optional values
		if (!mAppVersion.empty())		result += "&av=" + ci::Url::encode(mAppVersion);
		if (!mCustomQuery.empty())		result += mCustomQuery;
		if (mAnonymizeIp)				result += "&aip=1";
		if (mSessionControl == Start)	result += "&sc=start";
		if (mSessionControl == End)		result += "&sc=end";
		if (mCacheBuster)				result += "&z=" + std::to_string(rand()); // should be final parameter

		return result;
	}

	const double		mTimestamp;		//! Time in seconds; Set in constructor automatically

	const std::string	mAppName;		//! Mandatory GA application name (required by mobile app type properties)
	const std::string	mTrackingId;	//! Mandatory GA tracking ID
	const std::string	mClientId;		//! Mandatory GA client ID formatted as UUID according to http://www.ietf.org/rfc/rfc4122.txt
	const std::string	mVersion;		//! Mandatory GA API version
	const std::string	mType;			//! Mandatory GA Hit Type ('pageview', 'screenview', 'event', 'transaction', 'item', 'social', 'exception' or 'timing')

	// Additional optional values
	std::string			mCustomQuery	= "";		//! Optional custom query, e.g. to append custom dimensions: '&cd1=first-dim&cd2=second-dim'
	std::string			mAppVersion		= "";		//! Optional app version (shows up in reports)
	bool				mAnonymizeIp	= false;
	bool				mCacheBuster	= true;
	SessionControl		mSessionControl = None;
};

} // analytics namespace
} // bluecadet namespace
