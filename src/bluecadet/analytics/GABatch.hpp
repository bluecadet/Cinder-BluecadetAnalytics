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

#include "GAHit.hpp"

namespace bluecadet {
namespace analytics {

typedef std::shared_ptr<class GABatch> GABatchRef;

class GABatch {
public:

	static const size_t MAX_NUM_HITS	= 20;			//! 20 items max per batch
	//static const size_t MAX_HIT_SIZE	= 8 * 1014;		//! 8kb max per item
	//static const size_t MAX_BATCH_SIZE= 16 * 1024;	//! 16kb max per batch
	static const int MAX_DELAY_BETWEEN_ATTEMPTS = 300;	//! How long in seconds to wait until re-attempting to send this batch

	GABatch() {};
	~GABatch() {};

	std::string getPayloadString() const {
		std::string result = "";
		for (auto hit : mHits) {
			if (!result.empty()) result += "\n";
			result += hit->getPayloadString();
		}
		return result;
	};

	void addHit(GAHitRef hit) {
		mHits.push_back(hit);
	}
	bool canAddHit(GAHitRef hit) const { return mHits.size() < MAX_NUM_HITS; }
	bool isFull() const { return mHits.size() >= MAX_NUM_HITS; }
	bool isEmpty() const { return mHits.empty(); }
	size_t numHits() const { return mHits.size(); }

	//! Gets the age in seconds of the oldest hit or 0 if no hits added yet
	double getAge() { return mHits.empty() ? 0.0 : ci::app::getElapsedSeconds() - mHits.front()->mTimestamp; }

	double getTimeOfLastSendAttempt() const { return mTimeOfLastSendAttempt; }
	void setTimeOfLastSendAttempt(double timeOfLastSendAttempt) { mTimeOfLastSendAttempt = timeOfLastSendAttempt; }

	double getDelayUntilNextSendAttempt() const { return mDelayUntilNextSendAttempt; }
	void increaseDelayUntilNextSendAttempt() {
		if (mDelayUntilNextSendAttempt < (double)MAX_DELAY_BETWEEN_ATTEMPTS) {
			mDelayUntilNextSendAttempt *= 2.0;
		}
	}

protected:
	std::vector<GAHitRef> mHits;
	double mTimeOfLastSendAttempt = 0.0;
	double mDelayUntilNextSendAttempt = 1.0;
	
};

} // analytics namespace
} // bluecadet namespace
