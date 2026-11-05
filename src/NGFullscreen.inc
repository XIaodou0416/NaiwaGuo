static BOOL NGPlaybackViewVisible(UIView *view, UIWindow *window) {
    if (!view || !window || view.window != window || view.hidden || view.alpha < 0.01) return NO;
    for (UIView *node = view.superview; node; node = node.superview) if (node.hidden || node.alpha < 0.01) return NO;
    CGRect rect = [view convertRect:view.bounds toView:window];
    CGRect visible = CGRectIntersection(rect, window.bounds);
    if (CGRectIsNull(visible) || CGRectGetWidth(visible) < window.bounds.size.width * 0.75) return NO;
    return CGRectGetHeight(visible) >= MIN(240, window.bounds.size.height * 0.35);
}

static UIView *NGVisiblePlaybackPlayer(void) {
    UIView *best = nil;
    CGFloat bestArea = 0;
    for (UIWindow *window in NGCaptureWindows()) {
        if (window.windowLevel != UIWindowLevelNormal) continue;
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
        for (NSUInteger index = 0; index < queue.count && index < 12000; index++) {
            UIView *view = queue[index];
            NSString *name = NGClassName(view);
            if ([name isEqualToString:@"FQVShortVideoPlayerCellView"] ||
                [name isEqualToString:@"FQVShortVideoLandscapePlayerCellView"]) {
                if (NGPlaybackViewVisible(view, window)) {
                    CGRect rect = [view convertRect:view.bounds toView:window];
                    CGFloat area = CGRectGetWidth(rect) * CGRectGetHeight(rect);
                    if (area > bestArea) { best = view; bestArea = area; }
                }
            }
            [queue addObjectsFromArray:view.subviews];
        }
    }
    return best;
}
