- (void)nv_viewportAwaitResults {
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:10];
    while (![[self browserSession] searchResultsAreCurrent] && [deadline timeIntervalSinceNow] > 0) Pump();
    Check([[self browserSession] searchResultsAreCurrent] && ![[self browserSession] searchError],
        @"the real fuzzy worker completes the fixture search");
}

- (NSDictionary *)nv_viewportObserve:(NSString *)phase selected:(NSString *)selected note:(NoteObject *)note status:(BOOL)status {
    [[window contentView] layoutSubtreeIfNeeded];
    NVBrowserSession *session = [self browserSession];
    NSRect clip = [[notesScrollView contentView] bounds];
    NSInteger top = [notesTableView rowAtPoint:NSMakePoint(1, NSMinY(clip) + .1)];
    NSString *topKey = top >= 0 ? [session rowKeyAtIndex:(NSUInteger)top] : @"";
    CGFloat topOffset = top >= 0 ? NSMinY(clip) - NSMinY([notesTableView rectOfRow:top]) : 0;
    NSDictionary *row = @{@"phase": phase, @"pending": @([session searchPending]),
        @"current": @([session searchResultsAreCurrent]), @"status": [searchStatusField stringValue],
        @"windowWidth": @(NSWidth([window frame])), @"paneHeight": @(NSHeight([notesSubview bounds])),
        @"listHeight": @(NSHeight([notesScrollView frame])), @"clipHeight": @(NSHeight(clip)),
        @"clipY": @(NSMinY(clip)), @"documentHeight": @(NSHeight([notesTableView bounds])),
        @"selectedRow": @([notesTableView primarySelectedRow]),
        @"selectedKey": [self selectedSearchResultRowKey] ?: @"", @"topKey": topKey ?: @"",
        @"topOffset": @(topOffset), @"preview": @([self isViewingNote])};
    [ViewportMeasurements addObject:row]; NSLog(@"VIEWPORT %@", row);
    NSRect expected = [notesSubview bounds]; if (status) expected.size.height -= 24;
    Check(NSEqualRects([notesScrollView frame], expected),
        @"actual list reclaims all 24 status points except while progress is visible");
    Check([searchStatusField isHidden] == !status &&
        [[searchStatusField stringValue] isEqual:status ? NSLocalizedString(@"Searching…", nil) : @""],
        @"actual status visibility and text match the current transition");
    Check([[self selectedSearchResultRowKey] isEqual:selected] && [self selectedNoteObject] == note,
        @"layout preserves the selected result occurrence and its note");
    Check([[field stringValue] isEqual:@"viewport"] && [[session searchString] isEqual:@"viewport"],
        @"layout preserves the actual search query");
    return row;
}

- (void)nv_viewportCompare:(NSDictionary *)before after:(NSDictionary *)after {
    CGFloat maximum = MAX(0, [after[@"documentHeight"] doubleValue] - [after[@"clipHeight"] doubleValue]);
    CGFloat expected = MIN([before[@"clipY"] doubleValue], maximum);
    Check(fabs([after[@"clipY"] doubleValue] - expected) <= 1,
        @"viewport keeps its vertical origin or clamps only to the new document bottom");
    if (fabs(expected - [before[@"clipY"] doubleValue]) <= 1) {
        Check([after[@"topKey"] isEqual:before[@"topKey"]] &&
            fabs([after[@"topOffset"] doubleValue] - [before[@"topOffset"] doubleValue]) <= 1,
            @"unclamped viewport preserves the top result occurrence and its point offset");
    }
}

- (void)nv_viewportAwaitPreview {
    PreviewController *provider = [self valueForKey:@"previewController"];
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:12];
    while (([provider loading] || [[provider webView] isLoading] || ![provider renderedHTML]) &&
        [deadline timeIntervalSinceNow] > 0) Pump();
    Check(provider && ![provider loading] && ![[provider webView] isLoading] && [provider renderedHTML],
        @"the HTML boundary uses a completed actual preview");
}

- (void)nv_runTests {
    @try {
        [self nv_activateBrowser];
        NVApplicationController *app = [NVApplicationController sharedController];
        NotationController *library = [app library];
        [prefsController setAutoCompleteSearches:NO sender:self];
        [prefsController setShowNotesList:YES sender:nil];
        NSMutableArray *notes = [NSMutableArray array];
        for (NSUInteger i = 0; i < 48; i++) {
            NoteObject *note = MakeNote(library, [NSString stringWithFormat:@"Viewport %02lu", (unsigned long)i],
                [NSString stringWithFormat:@"<p>Viewport fixture %02lu. Preserve the source and selected result.</p>", (unsigned long)i]);
            [notes addObject:note];
        }
        [self searchForString:@"viewport" mode:@"fuzzy"]; [self nv_viewportAwaitResults];
        NVBrowserSession *session = [self browserSession];
        Check([session resultCount] == 96 && [session distinctResultNoteCount] == 48,
            @"48 actual notes produce 96 title and fuzzy result occurrences");
        ViewportDeliveries = [NSMutableArray new]; ViewportMeasurements = [NSMutableArray new];
        Method requestMethod = class_getInstanceMethod([NVSearchService class], @selector(requestForOwner:query:completion:));
        IMP originalRequest = method_getImplementation(requestMethod);
        IMP gate = imp_implementationWithBlock(^NSUInteger(NVSearchService *service, id owner, NSString *query, NVSearchCompletion completion) {
            return ((NSUInteger(*)(id,SEL,id,id,id))originalRequest)(service, @selector(requestForOwner:query:completion:), owner, query,
                ^(NVSearchResult *result, NSError *error) {
                    ViewportDelivery *delivery = [[[ViewportDelivery alloc] init] autorelease];
                    delivery->completion = [completion copy]; delivery->result = [result retain]; delivery->error = [error retain];
                    [ViewportDeliveries addObject:delivery];
                });
        });
        method_setImplementation(requestMethod, gate);
        if (getenv("NV_VIEWPORT_NEGATIVE")) {
            Method method = class_getInstanceMethod([AppController class], @selector(updateSearchAffordance));
            IMP original = method_getImplementation(method);
            method_setImplementation(method, imp_implementationWithBlock(^(AppController *browser) {
                ((void(*)(id,SEL))original)(browser, @selector(updateSearchAffordance));
                if (ViewportExpectReclaimedSpace && [[browser browserSession] searchResultsAreCurrent]) {
                    NSRect frame = [[browser valueForKey:@"notesSubview"] bounds]; frame.size.height -= 24;
                    [[browser valueForKey:@"notesScrollView"] setFrame:frame];
                }
            }));
        }

        for (NSString *scenario in @[@"source-middle", @"source-bottom", @"preview-middle"]) {
            BOOL preview = [scenario hasPrefix:@"preview"], bottom = [scenario hasSuffix:@"bottom"];
            [window setFrame:NSMakeRect(100, 100, 780, 660) display:YES]; [self setNotesListHeight:180]; Pump();
            NSUInteger selectedRow = bottom ? 95 : 64;
            [notesTableView selectRowAndScroll:selectedRow]; [self displayContentsForNoteAtIndex:selectedRow];
            if (preview) {
                NSMenuItem *choice = [[[NSMenuItem alloc] init] autorelease]; [choice setRepresentedObject:@"html"];
                [self selectPreviewMode:choice]; [self nv_viewportAwaitPreview];
            } else [self setViewingNote:NO];
            [self focusNoteBody]; Pump();
            NSClipView *clip = [notesScrollView contentView];
            CGFloat initialY = bottom ? MAX(0, NSHeight([notesTableView bounds]) - NSHeight([clip bounds])) :
                NSMinY([notesTableView rectOfRow:selectedRow]) - 12;
            [clip scrollToPoint:NSMakePoint(0, initialY)]; [notesScrollView reflectScrolledClipView:clip]; Pump();
            NSString *selected = [[self selectedSearchResultRowKey] copy];
            NoteObject *note = [[self selectedNoteObject] retain];
            Check([[session matchKindAtIndex:selectedRow] isEqual:@"fuzzy"] && [selected length],
                @"the measured selection is a specific fuzzy occurrence in the actual scrolled list");
            NSDictionary *before = [self nv_viewportObserve:[scenario stringByAppendingString:@"-baseline"] selected:selected note:note status:NO];
            NSUInteger deliveryIndex = [ViewportDeliveries count];
            [session refilterNotes];
            Check([session searchPending] && !searchStatusDelayElapsed,
                @"real refiltering starts in the silent pending state before its native timer");
            NSDictionary *silent = [self nv_viewportObserve:[scenario stringByAppendingString:@"-silent"] selected:selected note:note status:NO];
            [self nv_viewportCompare:before after:silent];
            NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:10];
            while (([ViewportDeliveries count] <= deliveryIndex || !searchStatusDelayElapsed) && [deadline timeIntervalSinceNow] > 0) Pump();
            Check([ViewportDeliveries count] > deliveryIndex && [session searchPending] && searchStatusDelayElapsed,
                @"actual fuzzy computation reaches the gate and the production progress timer fires");
            NSDictionary *progress = [self nv_viewportObserve:[scenario stringByAppendingString:@"-progress"] selected:selected note:note status:YES];
            [self nv_viewportCompare:silent after:progress];
            for (NSNumber *width in @[@1000, @600]) {
                NSRect frame = [window frame]; frame.size.width = [width doubleValue]; [window setFrame:frame display:YES]; Pump();
                NSDictionary *resized = [self nv_viewportObserve:[NSString stringWithFormat:@"%@-progress-width%@", scenario, width]
                    selected:selected note:note status:YES];
                [self nv_viewportCompare:progress after:resized]; progress = resized;
            }
            if (bottom) {
                CGFloat lastOrigin = MAX(0, NSHeight([notesTableView bounds]) - NSHeight([clip bounds]));
                [clip scrollToPoint:NSMakePoint(0, lastOrigin)]; [notesScrollView reflectScrolledClipView:clip]; Pump();
                progress = [self nv_viewportObserve:[scenario stringByAppendingString:@"-progress-bottom"] selected:selected note:note status:YES];
            }
            ViewportExpectReclaimedSpace = YES;
            [(ViewportDelivery *)[ViewportDeliveries objectAtIndex:deliveryIndex] deliver];
            [self nv_viewportAwaitResults];
            NSDictionary *completed = [self nv_viewportObserve:[scenario stringByAppendingString:@"-completed"] selected:selected note:note status:NO];
            [self nv_viewportCompare:progress after:completed];
            for (NSNumber *width in @[@780, @480]) {
                NSRect frame = [window frame]; frame.size.width = [width doubleValue]; [window setFrame:frame display:YES]; Pump();
                NSDictionary *resized = [self nv_viewportObserve:[NSString stringWithFormat:@"%@-completed-width%@", scenario, width]
                    selected:selected note:note status:NO];
                [self nv_viewportCompare:completed after:resized]; completed = resized;
            }
            Check([self isViewingNote] == preview && [session resultCount] == 96 && [session distinctResultNoteCount] == 48,
                @"the completed history preserves the body mode and all actual result occurrences");
            if (preview) [self nv_viewportAwaitPreview];
            [selected release]; [note release];
        }
        for (NSUInteger i = 0; i < [notes count]; i++)
            Check([[[notes[i] contentString] string] isEqual:[NSString stringWithFormat:@"<p>Viewport fixture %02lu. Preserve the source and selected result.</p>", (unsigned long)i]],
                @"view transitions preserve the exact source of each disposable note");
        Check([library totalNoteCount] == 48, @"the viewport histories preserve exactly 48 fixture notes");
        NSData *data = [NSJSONSerialization dataWithJSONObject:@{@"observations": ViewportMeasurements,
            @"checksBeforeArtifact": @(Checks)} options:NSJSONWritingPrettyPrinted error:NULL];
        Check([data writeToFile:[NSString stringWithUTF8String:getenv("NV_VIEWPORT_RESULT")] atomically:YES],
            @"the actual viewport measurements are saved");
        method_setImplementation(requestMethod, originalRequest); imp_removeBlock(gate);
        [ViewportDeliveries removeAllObjects]; [ViewportDeliveries release]; ViewportDeliveries = nil;
        NSLog(@"SEARCH_SUMMARY_VIEWPORT_PASS checks=%lu observations=%lu", (unsigned long)Checks, (unsigned long)[ViewportMeasurements count]);
        [self performSelector:@selector(nv_finishTests) withObject:nil afterDelay:.1];
    } @catch (NSException *exception) { NSLog(@"FAIL exception: %@\n%@", exception, [exception callStackSymbols]); exit(1); }
}
