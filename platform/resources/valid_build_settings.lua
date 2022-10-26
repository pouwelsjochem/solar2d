------------------------------------------------------------------------------
--
-- This file is part of the Corona game engine.
-- For overview and more information on licensing please refer to README.md 
-- Home page: https://github.com/coronalabs/corona
-- Contact: support@coronalabs.com
--
------------------------------------------------------------------------------

-- These are items we recognize but cannot validate the contents of
-- (generally because they're arbitrary).  Be sure to add items to
-- the settings table as well.
stoplist = {
	"plugins",
	"usesFeatures",
	"CFBundleURLTypes",
	"titleText",
	"intentFilters",
	"NSExceptionDomains",
	"UIRequiredDeviceCapabilities",
	"iCloud",
	"onDemandResources",
	"strings",
	"coronaActivityFlags",
	"entitlements",
	"xcassets",
}

settings =
{
	android =
	{
		onDemandResources = { },
		usesFeatures = { },
		intentFilters = { },
		usesPermissions = { "" },
		versionName = "",  -- Can be a string or number. String is preferred.
		installLocation = "",

		versionCode = "",
		usesExpansionFile = true,
		supportsTV = true,
		isGame = true,
		allowAppsReadOnlyAccessToFiles = false,
		minSdkVersion = "",
		largeHeap = true,
		googlePlayGamesAppId = "",
		mainIntentFilter =
		{
			categories =
			{
				"",
			},
		},
		permissions =
		{
			{
				name = "",
				description = "",
				icon = "",
				label = "",
				permissionGroup="",
				protectionLevel=""
			},
		},
		supportsScreens =
		{
			smallScreens  = true,
			normalScreens = true,
			largeScreens  = true,
			xlargeScreens = false,
			resizeable = true,   -- This is not a typo. Google expects an 'e' in "resizeable".
			anyDensity = true,
			requiresSmallestWidthDp = 1,
			compatibleWidthLimitDp = 1,
			largestWidthLimitDp = 1,
		},
		facebookAppId = "",  -- Both camel case and pascal case are accepted.
		FacebookAppID = "",

		applicationChildElements = { "" },
		manifestChildElements = { "" },
		apkFiles = { "" },
		strings = { },
		useGoogleServicesJson = true,
		coronaActivityFlags = { },
	},

	androidPermissions = { "DEPRECATED" },

	build = {
		custom = "",
		neverStripDebugInfo = true,
	},

	corona_sdk_simulator_path = "", -- used by Corona Editor

	excludeFiles =
	{
		iphone = { "", },
		ios = { "", },
		android = { "", },
		osx = { "", },
		macos = { "", },
		win32 = { "", },
		tvos = { "", },
		web = { "", },
		all = { "" },
	},

	iphone =
	{
		onDemandResources = { },
		iCloud = true,
		skipPNGCrush = true,
		xcassets = "",
		entitlements = { },
		plist =  -- most of these keys are probably valid for tvos too.
		{
			CFBundleId = "",
			CFBundleIconFile = "DEPRECATED",
			CFBundleIconFiles =
			{
				"",
			},
			CFBundleURLTypes =
			{
				CFBundleURLSchemes = {
					"",
				}
			},
			UISupportedInterfaceOrientations = { "CORONA" },
			UIApplicationExitsOnSuspend = false,
			LSApplicationQueriesSchemes = { "" },
			FacebookAppID = "",
			CFBundleIdentifier = "",
			CFBundleShortVersionString = "",
			CFBundleDisplayName = "",
			CFBundleName = "",
			CFBundleVersion = "",
			UILaunchImages = {
				{
					["UILaunchImageMinimumOSVersion"] = "",
					["UILaunchImageName"] = "",
					["UILaunchImageOrientation"] = "",
					["UILaunchImageSize"] = "",
				},
			},
			UILaunchStoryboardName = "", -- optional replacement for UILaunchImages
			UIPrerenderedIcon = true,
			MinimumOSVersion = "",
			UIRequiredDeviceCapabilities = { },
			UIViewControllerBasedStatusBarAppearance = false,
            CoronaUseIOS8LandscapeOnlyWorkaround = true,
			UIBackgroundModes = {'remote-notification'},
			CoronaWindowMovesWhenKeyboardAppears = false,
			CFBundleLocalizations = {
				"en",
			},
			SKAdNetworkItems = {
				{ SKAdNetworkIdentifier = "" },
			},
			NSAppTransportSecurity = {
				NSAllowsArbitraryLoads = false,
				NSAllowsArbitraryLoadsInMedia = false,
				NSAllowsArbitraryLoadsInWebContent = false,
				NSAllowsLocalNetworking = false,
				NSExceptionDomains = {},
			},
			NSAppleMusicUsageDescription = "",
			NSUserTrackingUsageDescription = "",
			NSBluetoothPeripheralUsageDescription = "",
			NSCalendarsUsageDescription = "",
			NSCameraUsageDescription = "",
			NSContactsUsageDescription = "",
			NSHealthShareUsageDescription = "",
			NSHealthUpdateUsageDescription = "",
			NSHomeKitUsageDescription = "",
			NSMicrophoneUsageDescription = "",
			NSMotionUsageDescription = "",
			NSPhotoLibraryUsageDescription = "",
			NSPhotoLibraryAddUsageDescription = "",
			NSRemindersUsageDescription = "",
			NSSiriUsageDescription = "",
			NSSpeechRecognitionUsageDescription = "",
			ITSAppUsesNonExemptEncryption = false,
			GooglePlayGamesOAuth2ClientId = "",
		},
	},

	osx = 
	{
		onDemandResources = { },
		iCloud = {},
		bundleResourcesDirectory = "",
		entitlements = { },
		plist =
		{
			CFBundleURLTypes = {
				{
					CFBundleURLName = "",
					CFBundleURLSchemes =
					{
						"",
					}
				}
			},
			CFBundleDocumentTypes =
			{
				{
					CFBundleTypeExtensions =
					{
						"png",
					},
					CFBundleTypeIconFile = "",
					CFBundleTypeName = "",
					CFBundleTypeRole = "",
					LSHandlerRank = "",
					LSItemContentTypes =
					{
						"",
					}
				}
			},
			NSHumanReadableCopyright = "",
			NSContactsUsageDescription = "",
			NSAppTransportSecurity = {
				NSAllowsArbitraryLoads = true,
				NSAllowsArbitraryLoadsInMedia = true,
				NSAllowsArbitraryLoadsInWebContent = true,
				NSAllowsLocalNetworking = true,
				NSExceptionDomains = {},
			},
			ITSAppUsesNonExemptEncryption = false,
			LSMinimumSystemVersion = "",
		}
	},

	buildQueue = "",

	plugins = 
	{
	},

	tvos =
	{
		iCloud = true,
		xcassets = "",
		onDemandResources = { },
		-- tvOS app icons require multiple layers, and must provide both a small and a large size.
		icon =
		{
			-- A collection of 400x240 pngs, in order of top to bottom.
			small =
			{
				"",
			},
			-- A collection of 1280x768 pngs, in order of top to bottom.
			large =
			{
				"",
			}
		},

		topShelfImage = "",
		topShelfImageWide = "",
		launchImage = "",

		plist = {
			UIApplicationExitsOnSuspend = true,
			NSVideoSubscriberAccountUsageDescription = "",
			ITSAppUsesNonExemptEncryption = false,
		},
	},

	win32 = {
		preferenceStorage = "",
		singleInstance = true,
	},

	window =
	{
		defaultMode = "",
		enableMinimizeButton = true,
		enableCloseButton = true,
		suspendWhenMinimized = true,
		defaultViewWidth = 1,
		defaultViewHeight = 1,
		titleText =
		{
			default = "",
		},
	},
}

-- forwards compatibility
settings.ios = settings.iphone
settings.macos = settings.osx
