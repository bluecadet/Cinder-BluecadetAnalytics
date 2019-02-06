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
#include "cinder/Utilities.h"

#include "GAHit.hpp"

namespace bluecadet {
namespace analytics {
typedef std::shared_ptr<class GAUserTiming> GAUserTimingRef;

// https://developers.google.com/analytics/devguides/collection/protocol/v1/devguide#usertiming
class GAUserTiming : public GAHit {
public:
	GAUserTiming(const std::string & appName, const std::string & trackingId, const std::string & clientId, const std::string & version,
		const std::string & category, const std::string & variable, int timeInMs, const std::string & label = "", const std::string & customQuery = "") :
		GAHit(appName, trackingId, clientId, version, "timing", customQuery),
		mUserTimingCategory(category),
		mUserTimingVariable(variable),
		mUserTimingTime(timeInMs),
		mUserTimingLabel(label)
	{}
	virtual ~GAUserTiming() {};

	virtual std::string getPayloadString() const override {
		std::string result = GAHit::getPayloadString();

		result += "&utc=" + ci::Url::encode(mUserTimingCategory);	// Timing category
		result += "&utv=" + ci::Url::encode(mUserTimingVariable);	// Timing variable
		result += "&utt=" + ci::toString(mUserTimingTime);			// Timing time

		// Optional values
		if (!mUserTimingLabel.empty())	result += "&utl=" + ci::Url::encode(mUserTimingLabel);	// Timing label

		return result;
	}

	const std::string	mUserTimingCategory;		//! Mandatory
	const std::string	mUserTimingVariable;		//! Mandatory
	const int			mUserTimingTime;			//! Mandatory (milliseconds)
	const std::string	mUserTimingLabel;			//! Optional; Empty string will be omitted
};

} // analytics namespace
} // bluecadet namespace
