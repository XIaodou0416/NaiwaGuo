#import "../src/Debug/NGDDebugExport.h"
#import "../src/Debug/NGDAudioTap.h"
#import "../src/Debug/NGDAudioSignal.h"
#import <stdatomic.h>

static atomic_bool NGDebugTestQueueRunning;
static uint64_t NGDebugTestSample;
static void NGDebugTestQueueCallback(void *context, AudioQueueRef queue, AudioQueueBufferRef buffer) {
    if (!atomic_load(&NGDebugTestQueueRunning)) return;
    float *samples=buffer->mAudioData;
    for (NSUInteger i=0;i<1024;i++) samples[i]=0.15f*sinf((float)(2*M_PI*440*(NGDebugTestSample++)/48000.0));
    buffer->mAudioDataByteSize=1024*sizeof(float);
    AudioQueueEnqueueBuffer(queue,buffer,0,NULL);
}
static NSString *NGDebugTestDocuments(void) { return NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES).firstObject; }
static void NGDebugTestWrite(NSString *mode,NSDictionary *result) { NGSmokeWrite([mode stringByAppendingString:@".json"],result); }

static void NGDebugTestExport(NGDDebugExportContext *context,NGDDebugExportMode mode,NSString *name,NSMutableDictionary *checks,dispatch_block_t finish) {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0), ^{
        NSError *error=nil; NGDDebugExportResult *result=NGDDebugCreateExport(context,mode,nil,&error);
        checks[@"exportCreated"]=@(result.zipURL!=nil && error==nil);
        NSDictionary *manifest=result ? [NSJSONSerialization JSONObjectWithData:[NSData dataWithContentsOfURL:[result.workingDirectoryURL URLByAppendingPathComponent:@"manifest.json"]] options:0 error:nil] : nil;
        checks[@"integrityComplete"]=@([manifest[@"integrity"][@"complete"] boolValue]);
        checks[@"correctSchema"]=@([manifest[@"schemaVersion"] isEqual:mode==NGDDebugExportModeAudio ? @"naiwaguo.audio-probe.v2" : @"naiwaguo.debug.v2"]);
        if (result) {
            NSURL *saved=[NSURL fileURLWithPath:[NGDebugTestDocuments() stringByAppendingPathComponent:[name stringByAppendingString:@".zip"]]];
            [NSFileManager.defaultManager removeItemAtURL:saved error:nil];
            checks[@"archiveCopied"]=@([NSFileManager.defaultManager copyItemAtURL:result.zipURL toURL:saved error:nil]);
            NGDDebugCleanupExport(result);
            checks[@"temporaryFilesCleaned"]=@(![NSFileManager.defaultManager fileExistsAtPath:result.zipURL.path] && ![NSFileManager.defaultManager fileExistsAtPath:result.workingDirectoryURL.path]);
        }
        dispatch_async(dispatch_get_main_queue(), ^{ if (finish) finish(); BOOL pass=YES; for (NSNumber *n in checks.allValues) pass &= n.boolValue; checks[@"passed"]=@(pass); NGDebugTestWrite(name,checks); });
    });
}

static void NGRunDebug003(UIWindow *window,NSString *mode) {
    [NSUserDefaults.standardUserDefaults setBool:YES forKey:NGPreferenceHUD];
    NGDAudioHooksInstall();
    BOOL late=[mode isEqual:@"debug-late-003"];
    NGDAudioProbeInstallIfEnabled(!late);
    NGStart();
    if (late) {
        BOOL rejected=!NGDAudioProbeHasLaunchCoverage() && !NGDAudioProbeStartCapture(@"playing");
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceHUD]; NGDAudioProbePreferenceDidChange(); NGDDebugInspectorRefreshOverlay();
        NGDebugTestWrite(mode,@{@"lateEnableRequiresRestart":@(rejected),@"disabledStopsRecording":@(!NGDAudioProbeIsEnabled()),@"passed":@(rejected && !NGDAudioProbeIsEnabled())}); return;
    }
    if ([mode isEqual:@"debug-page-003"]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.5*NSEC_PER_SEC),dispatch_get_main_queue(), ^{
            UIWindow *overlay=NGDDebugInspectorWindow();
            BOOL passThrough=[overlay hitTest:CGPointMake(10,window.bounds.size.height/2) withEvent:nil]==nil;
            UILabel *label=[[UILabel alloc] initWithFrame:CGRectMake(20,160,280,50)]; label.text=@"NaiwaGuo diagnostic fixture"; label.layer.cornerRadius=13; label.clipsToBounds=YES; [window.rootViewController.view addSubview:label];
            UIWindow *secondary=[[UIWindow alloc] initWithWindowScene:window.windowScene]; secondary.frame=window.bounds; secondary.rootViewController=[UIViewController new]; secondary.hidden=NO; secondary.windowLevel=2;
            UILabel *extra=[[UILabel alloc] initWithFrame:CGRectMake(20,210,240,35)]; extra.text=@"Second window fixture"; [secondary.rootViewController.view addSubview:extra];
            NGDDebugExportContext *context=NGDDebugCaptureContext(window,CGPointZero,nil); context.probeText=NGDTabBarProbeReport();
            NSData *tree=[NSJSONSerialization dataWithJSONObject:context.viewTreeJSON options:0 error:nil]; NSString *treeString=[[NSString alloc] initWithData:tree encoding:NSUTF8StringEncoding];
            BOOL excluded=YES; for (NSDictionary *w in context.windowsJSON) excluded &= ![w[@"class"] hasPrefix:@"NGDDebug"];
            NSMutableDictionary *checks=[@{@"wrenchVisible":@(overlay && !overlay.hidden),@"blankTouchesPassThrough":@(passThrough),@"allAppWindowsCaptured":@(context.windowsJSON.count>=2),@"overlayExcluded":@(excluded),@"textAndRoundedLayerCaptured":@([treeString containsString:@"diagnostic fixture"] && [treeString containsString:@"cornerRadius"]),@"secondWindowCaptured":@([treeString containsString:@"Second window fixture"]),@"screenshotCaptured":@(context.screenshotPNG.length>1000),@"classHeadersCollected":@(context.pageClassNames.count>4),@"hongguoProbeIncluded":@([context.probeText containsString:@"Hongguo"] && [context.probeText containsString:@"typography"])} mutableCopy];
            secondary.hidden=YES; [window makeKeyWindow]; [label removeFromSuperview];
            NGDebugTestExport(context,NGDDebugExportModePage,mode,checks,^{ [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceHUD]; NGDAudioProbePreferenceDidChange(); NGDDebugInspectorRefreshOverlay(); });
        }); return;
    }
    if ([mode isEqual:@"debug-ui-003"]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.5*NSEC_PER_SEC),dispatch_get_main_queue(), ^{ NGDDebugInspectorPresentMenu(); });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,1.3*NSEC_PER_SEC),dispatch_get_main_queue(), ^{
            UIWindow *overlay=NGDDebugInspectorWindow(); UIAlertController *menu=(id)overlay.rootViewController.presentedViewController;
            BOOL correct=[menu isKindOfClass:UIAlertController.class] && menu.actions.count==3 && [menu.actions[0].title isEqual:@"导出本页 zip"] && [menu.actions[1].title isEqual:@"导出音频专项 zip"];
            BOOL privacy=[menu.message containsString:@"三秒播放声音"];
            UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithBounds:window.bounds];
            UIImage *shot=[renderer imageWithActions:^(UIGraphicsImageRendererContext *c){ [window drawViewHierarchyInRect:window.bounds afterScreenUpdates:YES]; [overlay drawViewHierarchyInRect:overlay.bounds afterScreenUpdates:YES]; }];
            [UIImagePNGRepresentation(shot) writeToFile:[NGDebugTestDocuments() stringByAppendingPathComponent:@"debug-menu-003.png"] atomically:YES];
            [menu dismissViewControllerAnimated:NO completion:^{
                [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceHUD]; NGDAudioProbePreferenceDidChange(); NGDDebugInspectorRefreshOverlay();
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW,0.2*NSEC_PER_SEC),dispatch_get_main_queue(), ^{
                    NGDebugTestWrite(mode,@{@"twoUpstreamExportActions":@(correct),@"privacyNoticeShown":@(privacy),@"disableHidesWrench":@(overlay.hidden),@"originalKeyWindowRestored":@(window.isKeyWindow),@"passed":@(correct && privacy && overlay.hidden && window.isKeyWindow)});
                });
            }];
        }); return;
    }
    // A real AudioQueue renders synthetic Float32 data with output volume zero.
    // This tests the actual symbol wrapper without playing sound to the user.
    AudioStreamBasicDescription format={.mSampleRate=48000,.mFormatID=kAudioFormatLinearPCM,.mFormatFlags=kAudioFormatFlagsNativeFloatPacked,.mBytesPerPacket=4,.mFramesPerPacket=1,.mBytesPerFrame=4,.mChannelsPerFrame=1,.mBitsPerChannel=32};
    AudioQueueRef queue=NULL; OSStatus created=AudioQueueNewOutput(&format,NGDebugTestQueueCallback,NULL,NULL,NULL,0,&queue);
    atomic_store(&NGDebugTestQueueRunning,true);
    if (queue) { AudioQueueSetParameter(queue,kAudioQueueParam_Volume,0); for (NSUInteger i=0;i<3;i++) { AudioQueueBufferRef buffer=NULL; AudioQueueAllocateBuffer(queue,4096,&buffer); if (buffer) NGDebugTestQueueCallback(NULL,queue,buffer); } }
    OSStatus started=queue ? AudioQueueStart(queue,NULL) : -1;
    BOOL began=NGDAudioProbeStartCapture(@"muted-playing"); BOOL duplicateRejected=!NGDAudioProbeStartCapture(@"playing");
    NSMutableArray *snapshots=[NSMutableArray arrayWithObject:NGDAudioRuntimeMediaSnapshot()];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2.5*NSEC_PER_SEC),dispatch_get_main_queue(), ^{ [snapshots addObject:NGDAudioRuntimeMediaSnapshot()]; });
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,5*NSEC_PER_SEC),dispatch_get_main_queue(), ^{
        NGDAudioProbeStopCapture(); [snapshots addObject:NGDAudioRuntimeMediaSnapshot()];
        atomic_store(&NGDebugTestQueueRunning,false); if (queue) { AudioQueueStop(queue,true); AudioQueueDispose(queue,true); }
        NGDDebugExportContext *context=NGDDebugCaptureMetadataContext(window); context.probeText=NGDTabBarProbeReport(); NSDictionary *geometry=NGDAudioRuntimeGlassTarget();
        dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0), ^{
            context.audioCapture=NGDAudioProbeBuildCapture(snapshots,geometry); NGDAudioProbeCapture *audio=context.audioCapture;
            BOOL allHooks=YES; for (NSDictionary *group in NGDAudioHooksCoverageJSON()) for (NSDictionary *m in group[@"methods"]) if ([m[@"exists"] boolValue]) allHooks &= [m[@"hooked"] boolValue];
            NSMutableDictionary *checks=[@{@"nativeAudioQueueStarted":@(created==noErr && started==noErr),@"fiveSecondCaptureStarted":@(began),@"concurrentCaptureRejected":@(duplicateRejected),@"threePlaybackSnapshots":@(audio.mediaSnapshotsJSON.count==3),@"realPCMRecorded":@(audio.pcmFiles.count>0 && [audio.diagnosticsJSON[@"strongestPeak"] doubleValue]>0.1),@"signalAnalysisPassed":@([audio.diagnosticsJSON[@"syntheticValidation"][@"passed"] boolValue]),@"allAvailableObjCHooksInstalled":@(allHooks),@"backendAndTimelineRecorded":@(audio.backendsJSON.count>0 && audio.timelineJSONL.length>0),@"thirtyHzSignalRecorded":@(audio.signalJSONL.length>100),@"stoppedAfterFiveSeconds":@(!NGDAudioProbeIsCaptureActive())} mutableCopy];
            NGDebugTestExport(context,NGDDebugExportModeAudio,mode,checks,^{ [NSUserDefaults.standardUserDefaults setBool:NO forKey:NGPreferenceHUD]; NGDAudioProbePreferenceDidChange(); });
        });
    });
}
