#include <array>
#include <unordered_map>
#include <string>

struct UESetting {
	const char* commandName;
	const char* description;
	uint8_t type; // 1 - int, 2 - float
};

std::array UE4settingsArray = {
	UESetting{"t.MaxFPS",                                  "Caps FPS to the given value.  Set to <= 0 to be uncapped.", 2},

	UESetting{"r.LensFlareQuality",                        " 0: off but best for performance\n"
															" 1: low quality with good performance\n"
															" 2: good quality (default)\n"
															" 3: very good quality but bad performance", 1},

	UESetting{"r.BloomQuality",								" 0: off, no performance impact.\n"
															" 1: average quality, least performance impact.\n"
															" 2: average quality, least performance impact.\n"
															" 3: good quality.\n"
															" 4: good quality.\n"
															" 5: Best quality, most significant performance impact. (default)\n"
															">5: force experimental higher quality on mobile (can be quite slow on some hardware)", 1},

	UESetting{"r.SceneColorFringeQuality",					" 0: off but best for performance\n"
															" 1: 3 texture samples (default)n", 1},

	UESetting{"r.AmbientOcclusionRadiusScale",				"Allows to scale the ambient occlusion radius (SSAO).\n"
															" 0:off, 1.0:normal, <1:smaller, >1:larger", 2},

	UESetting{"r.ShadowQuality",							"Defines the shadow method which allows to adjust for quality or performance.\n"
															" 0:off, 1:low(unfiltered), 2:low .. 5:max (default)", 1},

	UESetting{"r.MotionBlurQuality",						"Defines the motion blur method which allows to adjust for quality or performance.\n"
															" 0:off, 1:low, 2:medium, 3:high (default), 4: very high", 1},
	
	UESetting{"r.PostProcessAAQuality",						"Defines the postprocess anti aliasing method which allows to adjust for quality or performance.\n"
															" 0:off, 1:very low (faster FXAA), 2:low (FXAA), 3:medium (faster TemporalAA), 4:high (default TemporalAA), 5:very high, 6:max", 1},
	
	UESetting{"r.DepthOfFieldQuality",						"Allows to adjust the depth of field quality. Currently only fully affects BokehDOF. GaussianDOF is either 0 for off, otherwise on.\n"
															" 0: Off\n"
															" 1: Low\n"
															" 2: high quality (default, adaptive, can be 4x slower)\n"
															" 3: very high quality, intended for non realtime cutscenes, CircleDOF only (slow)\n"
															" 4: extremely high quality, intended for non realtime cutscenes, CircleDOF only (very slow)", 1},
	
	UESetting{"r.ScreenPercentage",							"To render in lower resolution and upscale for better performance (combined up with the blenable post process setting).\n"
															"70 is a good value for low aliasing and performance, can be verified with 'show TestImage'\n"
															"in percent, >0 and <=100, larger numbers are possible (supersampling) but the downsampling quality is improvable.<0 is treated like 100.", 2},
	
	UESetting{"r.MSAA.CompositingSampleCount",				"Affects the render quality of the editor 3d objects.\n"
															" 1: no MSAA, lowest quality\n"
															" 2: 2x MSAA, medium quality (medium GPU memory consumption)\n"
															" 4: 4x MSAA, high quality (high GPU memory consumption)\n"
															" 8: 8x MSAA, very high quality (insane GPU memory consumption)", 1},
	
	UESetting{"r.MobileMSAA",								"Use MSAA instead of Temporal AA on mobile:\n"
															"1: Use Temporal AA (MSAA disabled)\n"
															"2: Use 2x MSAA (Temporal AA disabled)\n"
															"4: Use 4x MSAA (Temporal AA disabled)\n"
															"8: Use 8x MSAA (Temporal AA disabled)", 1},
	
	UESetting{"r.MSAACount", 								"Number of MSAA samples to use with the forward renderer.  Only used when MSAA is enabled in the rendering project settings.\n"
															"0: MSAA disabled (Temporal AA enabled)\n"
															"1: MSAA disabled\n"
															"2: Use 2x MSAA\n"
															"4: Use 4x MSAA8: Use 8x MSAA", 1},
	
	UESetting{"r.MaxAnisotropy",							"MaxAnisotropy should range from 1 to 16. Higher values mean better texure quality when using anisotropic filtering but at a cost to performance. Default is 4.", 1},

	UESetting{"r.ViewDistanceScale", 						"Controls the view distance scale. A primitive's MaxDrawDistance is scaled by this value.\n"
															"Higher values will increase view distance but at a performance cost.\n"
															"Default = 1.", 2},
	
	UESetting{"r.EyeAdaptationQuality",						"Defines the eye adaptation quality which allows to adjust for quality or performance.\n"
															"<=0: off (fastest)\n"
															"  1: low quality (e.g. non histogram based, not yet implemented)\n"
															"  2: normal quality (default)\n"
															"  3: high quality (e.g. screen position localized, not yet implemented)", 1},
	
	UESetting{"r.Shadow.DistanceScale",						"Scalability option to trade shadow distance versus performance for directional lights (clamped within a reasonable range).\n"
															"<1: shorter distance\n"
															" 1: normal (default)\n"
															">1: larger distance", 2},
	
	UESetting{"r.Tonemapper.GrainQuantization",				"0: low (minor performance benefit)\n"
															"1: high (default, with high frequency pixel pattern to fight 8 bit color quantization)", 1},
	
	UESetting{"r.SkyLightingQuality",						"Defines the sky lighting quality which allows to adjust for performance.\n"
															"<=0: off (fastest)\n"
															"  1: on", 1},
	
	UESetting{"r.DynamicRes.OperationMode",					"Select the operation mode for dynamic resolution.\n"
															" 0: Disabled (default);\n"
															" 1: Enable according to the game user settings;\n"
															" 2: Enable regardless of the game user settings.", 1},
	
	UESetting{"r.DynamicRes.MinScreenPercentage",			"Minimal screen percentage.", 2},

	UESetting{"r.DynamicRes.MaxScreenPercentage",			"Maximal screen percentage.", 2},

	UESetting{"r.DynamicRes.FrameTimeBudget",				"Frame's time budget in milliseconds.", 2},

	UESetting{"r.Tonemapper.Sharpen",						"Sharpening in the tonemapper (not for mobile), actual implementation is work in progress, clamped at 10\n"
															"   0: off(default)\n"
															" 0.5: half strength\n"
															"   1: full strength", 2},
	
	UESetting{"r.Tonemapper.Quality",						"Defines the Tonemapper Quality in the range 0..5\n"
															"Depending on the used settings we might pick a faster shader permutation\n"
															" 0: basic tonemapper only, lowest quality\n"
															" 1: + FilmContrast\n"
															" 2: + Vignette\n"
															" 3: + FilmShadowTintAmount\n"
															" 4: + Grain\n"
															" 5: + GrainJitter = full quality (default)", 1},
	
	UESetting{"r.Upscale.Quality",							"Defines the quality in which ScreenPercentage and WindowedFullscreen scales the 3d rendering.\n"
															" 0: Nearest filtering\n"
															" 1: Simple Bilinear\n"
															" 2: Directional blur with unsharp mask upsample.\n"
															" 3: 5-tap Catmull-Rom bicubic, approximating Lanczos 2. (default)\n"
															" 4: 13-tap Lanczos 3.\n"
															" 5: 36-tap Gaussian-filtered unsharp mask (very expensive, but good for extreme upsampling).", 1},
	
	/* The same description is used by r.SubsurfaceScattering, so we cannot reliably search for this setting
	UESetting{"r.Fog",										" 0: disabled\n"
															" 1: enabled (default)", 1}
	*/

	UESetting{"r.DisableDistortion",						"Prevents distortion effects from rendering.  Saves a full-screen framebuffer's worth of memory.", 1},

	UESetting{"r.TemporalAA.Upsampling",					"Whether to do primary screen percentage with temporal AA or not.\n"
															" 0: use spatial upscale pass independently of TAA (default);\n"
															" 1: TemporalAA performs spatial and temporal upscale as screen percentage method.", 1},
	
	UESetting{"r.SSR.Quality", 								"Whether to use screen space reflections and at what quality setting.\n"
															"(limits the setting in the post process settings which has a different scale)\n"
															"(costs performance, adds more visual realism but the technique has limits)\n"
															" 0: off (default)\n"
															" 1: low (no glossy)\n"
															" 2: medium (no glossy)\n"
															" 3: high (glossy/using roughness, few samples)\n"
															" 4: very high (likely too slow for real-time)", 1},
	
	/* The same description is used by Fog so we cannot reliably search for this setting
	UESetting{"r.SubsurfaceScattering",						" 0: disabled\n"
															" 1: enabled (default)", 1}
	*/

	UESetting{"r.SSS.Quality",								"Defines the quality of the recombine pass when using the SubsurfaceScatteringProfile shading model\n"
															" 0: low (faster, default)\n"
															" 1: high (sharper details but slower)\n"
															"-1: auto, 1 if TemporalAA is disabled (without TemporalAA the quality is more noticable)", 1},
	
	UESetting{"r.SSS.Scale",								"Affects the Screen space Separable subsurface scattering pass (use shadingmodel SubsurfaceProfile, get near to the object as the default)\n"
															"is human skin which only scatters about 1.2cm)\n"
															" 0: off (if there is no object on the screen using this pass it should automatically disable the post process pass)\n"
															"<1: scale scatter radius down (for testing)\n"
															" 1: use given radius form the Subsurface scattering asset (default)\n"
															">1: scale scatter radius up (for testing)", 2},
	
	UESetting{"r.RefractionQuality",						"Defines the distorion/refraction quality which allows to adjust for quality or performance.\n"
															"<=0: off (fastest)\n"
															"  1: low quality (not yet implemented)\n"
															"  2: normal quality (default)\n"
															"  3: high quality (e.g. color fringe, not yet implemented)", 1},
	
	UESetting{"r.MaxQualityMode",							"If set to 1, override certain system settings to highest quality regardless of performance impact", 1},

	UESetting{"r.Mobile.ShadingPath",						"0: Forward shading (default)1: Deferred shading", 1},

	UESetting{"r.LightFunctionQuality",						"Defines the light function quality which allows to adjust for quality or performance.\n"
															"<=0: off (fastest)\n"
															"  1: low quality (e.g. half res with blurring, not yet implemented)\n"
															"  2: normal quality (default)\n"
															"  3: high quality (e.g. super-sampled or colored, not yet implemented)", 1},

	UESetting{"r.SSGI.Enable",								"Whether to enable SSGI (defaults to 0).\n", 1},

	UESetting{"r.SSGI.Quality",								"Quality setting to control number of ray shot with SSGI, between 1 and 4 (defaults to 4).\n", 1},

	UESetting{"r.GTSyncType",								"Determines how the game thread syncs with the render thread, RHI thread and GPU.\n"
															"Syncing to the GPU swap chain flip allows for lower frame latency.\n"
															" 0 - Sync the game thread with the render thread (default).\n"
															" 1 - Sync the game thread with the RHI thread.\n"
															" 2 - Sync the game thread with the GPU swap chain flip (only on supported platforms).\n", 1},
	
	UESetting{"foliage.LODDistanceScale",					"Scale factor for the distance used in computing LOD for foliage.", 2},

	UESetting{"r.Streaming.FullyLoadUsedTextures",			"If non-zero, all used texture will be fully streamed in as fast as possible", 1},

	UESetting{"r.TonemapperFilm",							"Use new film tone mapper", 1},

	UESetting{"r.SecondaryScreenPercentage.GameViewport",	"Override secondary screen percentage for game viewport.\n"
															" 0: Compute secondary screen percentage = 100 / DPIScalefactor automaticaly (default);\n"
															" 1: override secondary screen percentage.", 2},
	
	UESetting{"r.VSync", 									"0: VSync is disabled.(default)\n"
															"1: VSync is enabled.", 1},
	
	UESetting{"rhi.SyncInterval",							"Determines the frequency of VSyncs in supported RHIs.\n"
															"This is in multiples of 16.66 on a 60hz display, but some platforms support higher refresh rates.\n"
															"Assuming 60fps, the values correspond to:\n"
															"  0 - Unlocked (present immediately)\n"
															"  1 - Present every vblank interval\n"
															"  2 - Present every 2 vblank intervals\n"
															"  3 - etc...\n", 1},
	
	UESetting{"r.DefaultFeature.AntiAliasing",				"Engine default (project setting) for AntiAliasingMethod is (postprocess volume/camera/game setting still can override)\n"
															" 0: off (no anti-aliasing)\n"
															" 1: FXAA (faster than TemporalAA but much more shimmering for non static cases)\n"
															" 2: TemporalAA (default)\n"
															" 3: MSAA (Forward shading only)", 1},
	
	UESetting{"r.TemporalAA.Algorithm",						"Algorithm to use for Temporal AA\n"
															" 0: Gen 4 TAAU (default)\n"
															" 1: Gen 5 TAAU (experimental)", 1},

	UESetting{"r.TemporalAA.HistoryScreenPercentage",		"Size of temporal AA's history.", 2},

	UESetting{"r.TemporalAASamples",						"Number of jittered positions for temporal AA (4, 8=default, 16, 32, 64).", 1},

	UESetting{"r.TemporalAACurrentFrameWeight",				"Weight of current frame's contribution to the history.  Low values cause blurriness and ghosting, high values fail to hide jittering.", 2},

	UESetting{"r.TemporalAAFilterSize",						"Size of the filter kernel. (1.0 = smoother, 0.0 = sharper but aliased).", 2},

	UESetting{"r.TemporalAA.AllowDownsampling",				"Allows half-resolution color buffer to be produced during TAA. Only possible when motion blur is off and when using compute shaders for post processing.", 1},

	UESetting{"r.TemporalAAUpsampleFiltered",				"Use filtering to fetch color history during TamporalAA upsampling (see AA_FILTERED define in TAA shader). Disabling this makes TAAU faster, but lower quality. ", 1},

	UESetting{"r.TemporalAA.R11G11B10History",				"Select the bitdepth of the history.", 1},

	UESetting{"r.TemporalAAPauseCorrect",					"Correct temporal AA in pause. This holds onto render targets longer preventing reuse and consumes more memory.", 1},

	UESetting{"r.TemporalAACatmullRom",						"Whether to use a Catmull-Rom filter kernel. Should be a bit sharper than Gaussian.", 1}
};

std::unordered_map<std::string, std::string> UE4alternativeDescriptions1 = {
	{"rhi.SyncInterval",									"Determines the frequency of VSyncs in supported RHIs.  0 - Unlocked\n"
															"  1 - 60 Hz (16.66 ms)\n"
															"  2 - 30 Hz (33.33 ms)\n"
															"  3 - 20 Hz (50.00 ms)\n"},

	{"r.Tonemapper.Sharpen",								"Sharpening in the tonemapper (not for ES2), actual implementation is work in progress, clamped at 10\n"
															"   0: off(default)\n"
															" 0.5: half strength\n"
															"   1: full strength"},
	
	{"r.Mobile.ShadingPath",								"0: Forward shading (default)\n"
															"1: Deferred shading"}
};

std::array UE5settingsArray = {
	UESetting{"r.AntiAliasingMethod",						"Engine default (project setting) for AntiAliasingMethod is (postprocess volume/camera/game setting still can override)\n"
															" 0: off (no anti-aliasing)\n"
															" 1: Fast Approximate Anti-Aliasing (FXAA)\n"
															" 2: Temporal Anti-Aliasing (TAA)\n"
															" 3: Multisample Anti-Aliasing (MSAA, Only available on the desktop forward renderer)\n"
															" 4: Temporal Super-Resolution (TSR, Default)", 1},

	UESetting{"r.TemporalAA.Quality",						"Quality of the main Temporal AA pass.\n"
															" 0: Disable input filtering;\n"
															" 1: Enable input filtering;\n"
															" 2: Enable input filtering, enable mobility based anti-ghosting (Default)", 1},

	UESetting{"r.TemporalAA.UseMobileConfig",				"1 to use mobile TAA config. This will disable groupshared caching of the color and depth buffers.\n"
															" 0: disabled (default);\n"
															" 1: enabled;\n", 1}
};