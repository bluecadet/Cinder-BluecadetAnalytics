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
#include "cinder/log.h"

#include "bluecadet/analytics/AnalyticsClient.h"
#include "cinder/Utilities.h"

 // Used to generate UUID; You can also generate one offline and hard code it in your app.
//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>
//#include <boost/uuid/uuid_io.hpp>

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace bluecadet::analytics;

class BasicSampleApp : public App {
public:
	void setup() override;
	void setupParams();
	void update() override;
	void draw() override;

	params::InterfaceGlRef mParams;
};

void BasicSampleApp::setup() {

	// UUID according to RFC 4122 spec (required). Can be hard coded into your app or randomly generated as below (boost optional).
	//string clientId = "00000000-0000-0000-0000-000000000000";
	//string clientId = boost::uuids::to_string(boost::uuids::random_generator()());
	string clientId = trim(loadString(loadUrl("https://www.uuidgenerator.net/api/version1"))); // trim since uuid gen uses extra characters
	
	// Change this to your app's GA ID (required). Important: GA API doesn't return any reporting errors, so make sure to validate your id.
	string gaId = "UA-74778167-1";

	// The name of your app (required).
	string appName = "Analytics Sample";

	// Can be used to filter events by app version later on (optional).
	string appVersion = "1.0";

	// Setup needs to be called before tracking hits.
	AnalyticsClient::getInstance()->setup(clientId, gaId, appName, appVersion);
	AnalyticsClient::getInstance()->setCustomQuery("&cb1=DefaultCustomQueryFromClient");

	setupParams();
}

void BasicSampleApp::setupParams() {
	mParams = params::InterfaceGl::create("BasicSampleApp", getWindowSize());
	mParams->setPosition(ivec2(0));

	mParams->addText("Config");
	mParams->addParam<string>("Client ID", [](string v) { AnalyticsClient::getInstance()->setClientId(v); }, [] { return AnalyticsClient::getInstance()->getClientId(); });
	mParams->addParam<string>("GA ID", [](string v) { AnalyticsClient::getInstance()->setGaId(v); }, [] { return AnalyticsClient::getInstance()->getGaId(); });
	mParams->addParam<string>("App Name", [](string v) { AnalyticsClient::getInstance()->setAppName(v); }, [] { return AnalyticsClient::getInstance()->getAppName(); });
	mParams->addParam<string>("App Version", [](string v) { AnalyticsClient::getInstance()->setAppVersion(v); }, [] { return AnalyticsClient::getInstance()->getAppVersion(); });

	mParams->addSeparator();
	mParams->addText("Tracking");
	mParams->addButton("Track 1 Event", [] {
		AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action");
	});
	mParams->addButton("Track 1 Event with Custom Dimensions A", [] {
		AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action", "", -1, "&cd1=first-a&cd2=second-a");
	});
	mParams->addButton("Track 1 Event with Custom Dimensions B", [] {
		AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action", "", -1, "&cd1=first-b&cd2=second-b");
	});
	mParams->addButton("Track 150 Events", [] {
		for (int i = 0; i < 150; ++i) {
			AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action");
		}
	});
	mParams->addButton("Track 1,500 Events", [] {
		for (int i = 0; i < 1500; ++i) {
			AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action");
		}
	});
	mParams->addButton("Track 1 Screen View", [] {
		AnalyticsClient::getInstance()->trackScreenView("Test Screen");
	});
	mParams->addButton("Track 150 Screen Views", [] {
		for (int i = 0; i < 150; ++i) {
			AnalyticsClient::getInstance()->trackScreenView("Test Screen");
		}
	});

	mParams->addSeparator();
	mParams->addButton("Quit", [this] { quit(); }, "key=q");
}

void BasicSampleApp::update() {
}

void BasicSampleApp::draw() {
	gl::clear(Color(0, 0, 0));
	mParams->draw();
}

CINDER_APP(BasicSampleApp, RendererGl)
