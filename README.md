# Cinder-BluecadetAnalytics

Client implementation for [Google Analaytics Measurement API v1](https://developers.google.com/analytics/devguides/collection/protocol/v1/). Built for interactive multiuser installations with long run times. Deployed to and optimized for highly trafficked installations at public spaces with 5,000+ tracked real world interactions per hour.

Tested in production on Windows 7, but also runs on MacOS and iOS. See [Google's dev guide](https://developers.google.com/analytics/devguides/collection/protocol/v1/devguide#event) for more API implementation info.

## Features

* Track events and screen views
* Extendable to support more hit types
* Multi-threaded HTTP requests using [Cinder-Asio](https://github.com/BanTheRewind/Cinder-Asio) and [Protocol](https://github.com/BanTheRewind/Cinder-Protocol)
* Offline support with automatic retries at increasing intervals
* Configured to stay within Google Analytics [quota limits](https://developers.google.com/analytics/devguides/collection/protocol/v1/limits-quotas)
	* Automatic session renewal
	* Automatic hit batching
	* Automatic cache busting

## Dependencies

[Cinder-Asio](https://github.com/BanTheRewind/Cinder-Asio) is a required dependency that needs to be installed in _Cinder/blocks_. [Protocol](https://github.com/BanTheRewind/Cinder-Protocol) is included in this repo since it's not set up as a Cinder block.

## Usage

```c++
string clientId = "00000000-0000-0000-0000-000000000000"; // required
string gaId = "UA-00000000-0"; // required
string appName = "My App"; // required

AnalyticsClient::getInstance()->setup(clientId, gaId, appName);
AnalyticsClient::getInstance()->trackScreenView("Test Screen");
AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action");
```

## Notes on Client ID

The client ID is used to identify unique clients and measures as unique users in Google Analytics. The client ID is a UUID according to [RFC 4122 spec](http://www.ietf.org/rfc/rfc4122.txt), which can be pre-generated and hard coded into your app (e.g. using [online tools](https://www.guidgenerator.com/online-guid-generator.aspx)) or randomly generated at run time (e.g. using boost).

Example UUID generation using boost:

```c++
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

//...

string clientId = boost::uuids::to_string(boost::uuids::random_generator()());
```

### Single User Mode

If your app only tracks as a single user, you can just use  the shared instance with your GA ID and client ID. All hits sent to GA will track under the same unique user.

```c++
AnalyticsClient::getInstance()->setup(clientId, gaId, appName, appVersion);
AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action");
AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action");
AnalyticsClient::getInstance()->trackEvent("Test Category", "Test Action");
```

This will create three event hits in total, each by the same user.

See the [AnalyticsSample](samples/AnalyticsSample) project for more details.

### Multi User Mode

If your app should track multiple users individually, you can create one client per user, each with their own unique client ID:

```c++
mClientA = AnalyticsClientRef(new AnalyticsClient());
mClientB = AnalyticsClientRef(new AnalyticsClient());
mClientC = AnalyticsClientRef(new AnalyticsClient());

mClientA->setup(clientIdA, gaId, appName, appVersion);
mClientB->setup(clientIdB, gaId, appName, appVersion);
mClientC->setup(clientIdC, gaId, appName, appVersion);

mClientA->trackEvent("Test Category", "Test Action");
mClientB->trackEvent("Test Category", "Test Action");
mClientC->trackEvent("Test Category", "Test Action");
```

This will create three event hits in total, each by individual users.

See the [MultiUserSample](samples/MultiUserSample) project for more details.

## Notes on Error Reporting

The Google Analytics API does not return any form of errors when reporting hits via HTTP. In fact, it returns `200 OK` for every request, regardless of its data and format, so make sure that your GA ID and client ID are correctly formatted.

You can test if events and screen views are coming in correctly by looking at the [Real Time](https://support.google.com/analytics/answer/1638635?hl=en) view in your GA dashboard.

See [Google's hit validation notes](https://developers.google.com/analytics/devguides/collection/protocol/v1/validating-hits) for more info.

## Notes on Sessions and Quotas

Google Analytics has a few [limitations](https://developers.google.com/analytics/devguides/collection/protocol/v1/limits-quotas) that make tracking of high amounts of hits on multi-user apps difficult. For example, each session is limited to 500 hits. This Cinder block automatically renews sessions once 400 hits have been reached to stay under this limit, but that means that session data in GA is not really meaningful.

This automatic renewal can be disabled using `setAutoSessionsEnabled(false)`.

## Unimplemented Features

The following features are nice to have and would cover relatively rare edge cases (e.g. payload size with reasonable events/screen views should never reach the limits below).

- Check for batch size limit of 16K bytes ([reference](https://developers.google.com/analytics/devguides/collection/protocol/v1/devguide#batch-limitations))
- Check for hit size limit of 8K bytes ([reference](https://developers.google.com/analytics/devguides/collection/protocol/v1/devguide#batch-limitations))
- Send remaining batches before app quits (currently all remaining batches will be discarded)

## Version Notes

* Version 1.0.0
* Tested with Cinder `0.9.1dev` commit [0b24d643e3](https://github.com/cinder/Cinder/commit/0b24d643e3b19a4ae6875b92899bae9376f7a64a)
