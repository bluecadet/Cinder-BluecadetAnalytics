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

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "cinder/params/Params.h"

// Used to generate UUID; You can also generate one offline and hard code it in your app.
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "bluecadet/analytics/AnalyticsClient.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace bluecadet::analytics;

class MultiUserSampleApp : public App {
public:
	void setup() override;
	void setupParams();
	void update() override;
	void draw() override;
	params::InterfaceGlRef mParams;
	
	AnalyticsClientRef mClientA;
	AnalyticsClientRef mClientB;
	AnalyticsClientRef mClientC;
};

void MultiUserSampleApp::setup() {
	
	// Change this to your app's GA ID (required). Important: GA API doesn't return any reporting errors, so make sure to validate your id.
	string gaId = "UA-00000000-0";
	
	// The name of your app (required).
	string appName = "Analytics Sample";
	
	// Can be used to filter events by app version later on (optional).
	string appVersion = "1.0";
	
	// Create multiple clients if you want to track individual users in a multi-user app
	mClientA = AnalyticsClientRef(new AnalyticsClient());
	mClientB = AnalyticsClientRef(new AnalyticsClient());
	mClientC = AnalyticsClientRef(new AnalyticsClient());
	
	// Tracking events or screen views on these clients should measure as an active user per client ID in GA
	mClientA->setup(gaId, boost::uuids::to_string(boost::uuids::random_generator()()), appName, appVersion);
	mClientB->setup(gaId, boost::uuids::to_string(boost::uuids::random_generator()()), appName, appVersion);
	mClientC->setup(gaId, boost::uuids::to_string(boost::uuids::random_generator()()), appName, appVersion);
	
	setupParams();
}

void MultiUserSampleApp::setupParams() {
	mParams = params::InterfaceGl::create("MultiUserSampleApp", ivec2(256, 256));
	
	mParams->addButton("Track 1 Event on Client A", [this] {
		mClientA->trackEvent("Test Category", "Test Action");
	});
	mParams->addButton("Track 1 Event on Client B", [this] {
		mClientB->trackEvent("Test Category", "Test Action");
	});
	mParams->addButton("Track 1 Event on Client C", [this] {
		mClientC->trackEvent("Test Category", "Test Action");
	});
	
	mParams->addSeparator();
	mParams->addButton("Quit", [this] { quit(); }, "key=q");
}

void MultiUserSampleApp::update() {
}

void MultiUserSampleApp::draw() {
	gl::clear(Color(0, 0, 0));
	mParams->draw();
}

CINDER_APP( MultiUserSampleApp, RendererGl )
