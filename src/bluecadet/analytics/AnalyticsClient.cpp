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

#include "AnalyticsClient.h"

#include "cinder/Log.h"

#include "GAEvent.hpp"
#include "GAScreenView.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace analytics {

AnalyticsClient::AnalyticsClient() :
	mThreadManager(make_shared<utils::ThreadManager>()),
	mCacheBusterEnabled(true),
	mAutoSessionsEnabled(true),
	mMaxHitsPerSession(400), // stay below 500 events per session limit
	mHitsInCurrentSession(0),
	mGaApiVersion("1")
{
}

AnalyticsClient::~AnalyticsClient() {
}

void AnalyticsClient::setup(string clientId, string gaId, string appName, string appVersion, int numThreads, double maxBatchAge, int maxBatchesPerCycle) {
	destroy();
	
	mAppName = appName;
	mAppVersion = appVersion;
	mGaId = gaId;
	mClientId = clientId;
	mMaxBatchAge = maxBatchAge;
	mMaxBatchesPerCycle = maxBatchesPerCycle;
	mThreadManager->setup(numThreads);
	
	CI_LOG_I("Client set up with GA ID '" << mGaId << "' and client ID '" << mClientId << "'");
	
	mUpdateConnection = AppBase::get()->getSignalUpdate().connect([=] {
		// process batches regularly on sub thread
		mThreadManager->addTask([this] {
			processBatches();
		});
	});
}

void AnalyticsClient::destroy() {
	mThreadManager->destroy();
	
	if (mUpdateConnection.isConnected()) {
		mUpdateConnection.disconnect();
	}
	
	lock_guard<mutex> lock(mBatchMutex);
	
	mBatchQueue.clear();
	mCurrentBatch = nullptr;
}

void AnalyticsClient::trackEvent(const string & category, const string & action, const string & label, const int value, const std::string & customQuery) {
	GAEventRef event = make_shared<GAEvent>(mAppName, mGaId, mClientId, mGaApiVersion, category, action, label, value, customQuery);
	trackHit(event);
}

void AnalyticsClient::trackScreenView(const string & screenName, const std::string & customQuery) {
	GAScreenViewRef screenView = make_shared<GAScreenView>(mAppName, mGaId, mClientId, mGaApiVersion, screenName, customQuery);
	trackHit(screenView);
}

void AnalyticsClient::trackHit(GAHitRef hit) {

	// optional hit parameters
	hit->mAppVersion = mAppVersion;
	hit->mCacheBuster = mCacheBusterEnabled;

	// handle session control to stay within quotas
	if (mAutoSessionsEnabled) {

		if (mHitsInCurrentSession <= 0) {
			hit->mSessionControl = GAHit::SessionControl::Start;
			CI_LOG_V("Starting a new session");
		}

		mHitsInCurrentSession++;

		if (mHitsInCurrentSession >= mMaxHitsPerSession) {
			hit->mSessionControl = GAHit::SessionControl::End;
			CI_LOG_V("Ended session after " << to_string(mHitsInCurrentSession) << " hits");
			mHitsInCurrentSession = 0;
		}
	}

	mThreadManager->addTask([=] {
		lock_guard<mutex> lock(mBatchMutex);

		// ensure we have a batch that can fit our hit
		if (!mCurrentBatch || !mCurrentBatch->canAddHit(hit)) {

			if (mCurrentBatch) {
				// buffer current batch for sending
				mBatchQueue.push_back(mCurrentBatch);
			}

			// new batch
			mCurrentBatch = make_shared<GABatch>();

		}

		mCurrentBatch->addHit(hit);
	});
}

void AnalyticsClient::processBatches() {

	lock_guard<mutex> lock(mBatchMutex);

	// check if we should send the current batch
	if (mCurrentBatch && (mCurrentBatch->isFull() || mCurrentBatch->getAge() >= mMaxBatchAge)) {
		mBatchQueue.push_back(mCurrentBatch);
		mCurrentBatch = nullptr;
	}

	const double currentTime = getElapsedSeconds();
	const int maxBatchesToSend = min((int)mBatchQueue.size(), mMaxBatchesPerCycle);
	int numHitsSent = 0;
	int numBatchesSent = 0;

	// process batch queue and send as many as we can
	for (int i = 0; i < maxBatchesToSend; ++i) {
		auto batch = mBatchQueue.front();

		const double timeSinceLastAttempt = currentTime - batch->getTimeOfLastSendAttempt();
		if (timeSinceLastAttempt < batch->getDelayUntilNextSendAttempt()) {
			continue; // wait until we can try again
		}

		// send batch
		sendBatch(batch);

		numBatchesSent++;
		numHitsSent += (int)batch->numHits();

		// remove batch
		mBatchQueue.pop_front();
	}

	if (numBatchesSent > 0) {
		CI_LOG_V("Sending " << to_string(numBatchesSent) << " batches with a total of " <<
				 to_string(numHitsSent) << " hits (" << to_string(mBatchQueue.size()) << " batches remaining)");
	}
}

void AnalyticsClient::sendBatch(GABatchRef batch) {
	mThreadManager->addTask([=] {
		const string body = batch->getPayloadString();
		
		utils::UrlRequest::Options options;
		options.method = utils::UrlRequest::Method::POST;
		options.setBodyText(body);
		utils::UrlRequestRef request = utils::UrlRequest::create(mGaBaseUrl, mGaBatchUri, options);

		// save and send request
		lock_guard<mutex> lock(mRequestMutex);
		mPendingRequests.insert(request);
		request->connect([=] (utils::UrlRequestRef request) {
			handleBatchRequestCompleted(batch, request);
		});
	});
}

void AnalyticsClient::handleBatchRequestCompleted(GABatchRef batch, utils::UrlRequestRef request) {
	if (!request || !request->wasSuccessful()) {

		// increase delay until next attempt
		batch->setTimeOfLastSendAttempt(getElapsedSeconds());
		batch->increaseDelayUntilNextSendAttempt();
		
		const string reason = request ? request->getHttpResponse().getReason() : "(unknown reason)";
		CI_LOG_W("Batch failed to send: " << reason <<
				 " - attempting again in " << to_string(batch->getDelayUntilNextSendAttempt()) << " seconds");

		// push batch back onto queue to retry
		lock_guard<mutex> lock(mBatchMutex);
		mBatchQueue.push_front(batch);
	}

	AppBase::get()->dispatchAsync([=] {
		// discard request asynchronously (can't remove it directly from our callback)
		lock_guard<mutex> lock(mRequestMutex);
		auto it = mPendingRequests.find(request);
		if (it != mPendingRequests.end()) {
			mPendingRequests.erase(it);
		}
	});
}

} // analytics namespace
} // bluecadet namespace
