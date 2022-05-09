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

#include "GABatch.hpp"
#include "utils/ThreadManager.h"
#include "utils/UrlRequest.h"
#include "firebase/analytics.h"

namespace bluecadet {
namespace analytics {

typedef std::shared_ptr<class AnalyticsClient> AnalyticsClientRef;
	
/*!
 Google Analytics API v1 client.
 
 Provides static shared instance for convenience, but can be instantiated freely.
 
 See:
 - https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters
 - https://developers.google.com/analytics/devguides/collection/protocol/v1/devguide#event
 */
class AnalyticsClient {

public:
	
	
	//! Shared instance for singleton use. Singleton is not enforced, so you can still create multiple instances per app.
	static AnalyticsClientRef getInstance() {
		static AnalyticsClientRef instance = nullptr;
		if (!instance) {
			instance = AnalyticsClientRef(new AnalyticsClient());
		}
		return instance;
	}

	AnalyticsClient();
	virtual ~AnalyticsClient();

	
	//! Adds client to cinder update loop and automatically starts processing events and batches
	//! setup() and destroy() are symmetrical and can be called repeatedly.
	void setup(std::string clientId, std::string gaId, std::string appName, std::string appVersion = "", int numThreads = 1, double maxBatchAge = 4.0, int maxBatchesPerCycle = 8);
	
	//! Removes client from the update loop and clears any remaining hits.
	//! setup() and destroy() are symmetrical and can be called repeatedly.
	void destroy();
	
	
	
	//! Tracks a single event. May batch this hit with other hits and delay actually sending it.
	//! Any custom query will be added to this client's default custom query.
	void trackEvent(const std::string & category, const std::string & action, const std::string & label = "", const int value = -1, const std::string & customQuery = "");

	//! Tracks a single screen view. May batch this hit with other hits and delay actually sending it.
	//! Any custom query will be added to this client's default custom query.
	void trackScreenView(const std::string & screenName, const std::string & customQuery = "");

	//! Tracks an instance of a user timing hit and batches it with other hits if possible. Time is in MILLISECONDS
	void trackUserTiming(const std::string & category, const std::string & variable, const int timeInMs, const std::string & label = "", const std::string & customQuery = "");

	//! Tracks an instance of a base hit type and batches it with other hits if possible
	void trackHit(GAHitRef hit);
	
	
	
	//! Google Analytics ID (Required)
	std::string getGaId() const { return mGaId; }
	void setGaId(const std::string gaId) { mGaId = gaId; }
	
	//! Client ID in RFC 4122 UUID format (Required).
	//! See https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters#cid and http://www.ietf.org/rfc/rfc4122.txt
	std::string getClientId() const { return mClientId; }
	void setClientId(const std::string clientId) { mClientId = clientId; }
	
	//! Application name (Required).
	//! See https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters#an
	std::string getAppName() const { return mAppName; }
	void setAppName(const std::string appName) { mAppName = appName; }
	
	//! Application version (Optional).
	//! See https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters#av
	std::string getAppVersion() const { return mAppVersion; }
	void setAppVersion(const std::string appVersion) { mAppVersion = appVersion; }
	
	//! Additional query parameters to be appended to each hit payload (Optional).
	//! E.g. &cd1=myCustomDimensionValue
	std::string getCustomQuery() const { return mCustomQuery; }
	void setCustomQuery(const std::string customQuery) { mCustomQuery = customQuery; }  
	
	//! Automatically start/end sessions to stay within hits per session quota. Defaults to true.
	//! See https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters#sc)
	bool getAutoSessionsEnabled() const { return mAutoSessionsEnabled; }
	void setAutoSessionsEnabled(const bool value) { mAutoSessionsEnabled = value; }
	
	//! The maximum number of hits used per session if getAutoSessionsEnabled() is true. Defaults to 400.
	//! See https://developers.google.com/analytics/devguides/collection/protocol/v1/limits-quotas
	int getMaxHitsPerSession() const { return mMaxHitsPerSession; }
	void setMaxHitsPerSession(const int value) { mMaxHitsPerSession = value; }

	//! Add cache buster parameters to individual hits. Defaults to true.
	//! See https://developers.google.com/analytics/devguides/collection/protocol/v1/parameters#z
	bool getCacheBusterEnabled() const { return mCacheBusterEnabled; }
	void setCacheBusterEnabled(const bool value) { mCacheBusterEnabled = value; }

	
	
protected:
	
	//! Determines which batches are ready for sending and sends those. Called by update().
	void			processBatches();
	
	//! Attempts to send batch; Discards it on success, adds it back to front of the queue on failure.
	
	void			sendBatch(GABatchRef batch);
	//! Called by requests once they complete. Moves the batch back to the queue on failure.
	void			handleBatchRequestCompleted(GABatchRef batch, utils::UrlRequestRef request);
	
	
	
	// Threading
	std::mutex				mBatchMutex;
	std::mutex				mRequestMutex;
	utils::ThreadManagerRef	mThreadManager;
	ci::signals::ScopedConnection mUpdateConnection;
	
	
	
	// Batch management
	int						mMaxBatchesPerCycle;	//! Maximum number of batches to send in one cycle (i.e. one update frame)
	double					mMaxBatchAge;			//! Maximum time in seconds waited until a batch is sent off

	int						mHitsInCurrentSession;
	int						mMaxHitsPerSession;		//! GA has a limit of 500 hits per session. See https://developers.google.com/analytics/devguides/collection/protocol/v1/limits-quotas
	bool					mAutoSessionsEnabled;
	bool					mCacheBusterEnabled;
	
	std::deque<GABatchRef>	mBatchQueue;			//! Batches ready for sending
	GABatchRef				mCurrentBatch;			//! The current batch to capture events
	std::set<utils::UrlRequestRef>	mPendingRequests;
	
	
	
	// Google Analytics
	std::string				mGaApiVersion;	//! Google Analytics API version
	std::string				mGaId;			//! Google Analytics Tracking ID
	std::string				mAppName;		//! Google Analytics App Name (required for mobile app type properties)
	std::string				mAppVersion;	//! Google Analytics App Version (optional, shows up in reports and is filterable)
	std::string				mClientId;		//! Required client ID formatted as UUID according to http://www.ietf.org/rfc/rfc4122.txt
	std::string				mGaBaseUrl	= "www.google-analytics.com";
	std::string				mGaBatchUri	= "/batch";
	std::string				mCustomQuery = "";

	std::string				mAppInstanceId;
};

} // analytics namespace
} // bluecadet namespace
