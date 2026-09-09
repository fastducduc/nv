- (void)nv_o2Await {
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:5];
    while (![[self browserSession] searchResultsAreCurrent] && [deadline timeIntervalSinceNow] > 0) Pump();
    Check([[self browserSession] searchResultsAreCurrent], @"the real search completes before lifecycle assertions");
}
- (NSArray *)nv_o2Identities {
    // Pointer values do not extend the lifetime of the controls under observation.
    return @[[NSValue valueWithPointer:searchStatusField], [NSValue valueWithPointer:createNoteButton],
             [NSValue valueWithPointer:notesScrollView], [NSValue valueWithPointer:notesSubview]];
}
- (void)nv_o2Completed:(NSString *)query note:(NoteObject *)note identities:(NSArray *)identities visible:(BOOL)visible {
    Check([[self nv_o2Identities] isEqual:identities] && [searchStatusField superview] == notesSubview &&
        [createNoteButton superview] == notesSubview, @"lifecycle transitions reuse the status and action controls in the original list container");
    Check([searchStatusField isHidden] && [[searchStatusField stringValue] length] == 0 &&
        [[searchStatusField toolTip] length] == 0 && [[field toolTip] length] == 0,
        @"completed and empty searches retain no status text or tooltip");
    Check([[[browserSplitController splitViewItems] firstObject] isCollapsed] == !visible,
        @"native split-view visibility follows the notes-list preference");
    if (visible) {
        Check(NSEqualRects([notesScrollView frame], [notesSubview bounds]), @"visible notes list has no reserved summary gap");
    }
    Check([[field stringValue] isEqual:query] && [[[self browserSession] searchString] isEqual:query] &&
        [self selectedNoteObject] == note, @"collapse and peer lifecycle preserve each browser query and selected note");
    Check([createNoteButton isHidden], @"nonempty results and empty queries keep the Create action hidden");
}
- (void)nv_o2Prepare:(NSString *)query note:(NoteObject *)note {
    [self searchForString:query mode:@"fuzzy"];
    [self nv_o2Await]; [self revealNote:note options:0];
    [self nv_activateBrowser]; [self bringFocusToControlField:self];
    Check([field currentEditor] != nil, @"the lifecycle starts with a native Search field editor");
}
- (void)nv_o2FocusPreserved {
    Check([field currentEditor] != nil && [window firstResponder] == [field currentEditor],
        @"notes-list collapse and show preserve active Search editing");
}
- (void)nv_o2Layout {
    [[[window contentView] superview] layoutSubtreeIfNeeded];
    Pump();
}
- (void)nv_runTests {
    @try {
        NVSummaryDeaths = [[NSMutableDictionary alloc] init];
        NVApplicationController *app = [NVApplicationController sharedController];
        NotationController *library = [app library];
        [prefsController setAutoCompleteSearches:NO sender:self];
        [prefsController setShowNotesList:YES sender:nil];
        NoteObject *anchor = [MakeNote(library, @"Anchor", @"anchor shared source") retain];
        NoteObject *other = [MakeNote(library, @"Other", @"other source") retain];
        NSArray *primaryIdentities = [[self nv_o2Identities] copy];
        NVNoteEditingSession *session = [app editingSessionForNote:anchor];
        if (getenv("NV_SUMMARY_GAP_CONTROL")) {
            Method method = class_getInstanceMethod([AppController class], @selector(updateSearchAffordance));
            IMP original = method_getImplementation(method);
            method_setImplementation(method, imp_implementationWithBlock(^(AppController *browser) {
                ((void (*)(id, SEL))original)(browser, @selector(updateSearchAffordance));
                NVBrowserSession *session = [browser browserSession];
                if ([session searchResultsAreCurrent] && [[session searchMode] isEqual:@"fuzzy"] && [session hasSearchTerms]) {
                    // Keep the label hidden but restore only the discarded 24-point gap.
                    NSRect frame = [[browser valueForKey:@"notesSubview"] bounds];
                    frame.size.height = MAX(0, frame.size.height - 24);
                    [[browser valueForKey:@"notesScrollView"] setFrame:frame];
                }
            }));
        }
        for (NSUInteger cycle = 0; cycle < 2; cycle++) {
            NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
            NSString *primaryQuery = cycle ? @"" : @"anchor";
            NSString *peerQuery = cycle ? @"anchor" : @"";
            [self nv_o2Prepare:primaryQuery note:anchor];
            [self nv_o2Layout];
            [self nv_o2Completed:primaryQuery note:anchor identities:primaryIdentities visible:YES];
            Check([[[session textStorage] layoutManagers] count] == 1, @"the surviving browser owns one shared-source layout manager");
            [app newWindow:self]; Pump();
            AppController *peer = [[app browserControllers] lastObject];
            Check(peer != self, @"a disposable peer opens over the existing library");
            NVSummaryObserve(peer, @"controller");
            NVSummaryObserve([peer valueForKey:@"searchStatusField"], @"status");
            NVSummaryObserve([peer valueForKey:@"createNoteButton"], @"action");
            NSArray *peerIdentities = [[peer nv_o2Identities] copy];
            [peer nv_o2Prepare:peerQuery note:anchor];
            [peer nv_o2Layout];
            [self nv_o2Completed:primaryQuery note:anchor identities:primaryIdentities visible:YES];
            [peer nv_o2Completed:peerQuery note:anchor identities:peerIdentities visible:YES];
            Check([peer sharedNotationController] == library && [[[session textStorage] layoutManagers] count] == 2,
                @"the peer shares the library and adds only its own layout manager");
            NSRect primaryFrame = [window frame], peerFrame = [[peer window] frame];
            for (NSUInteger toggle = 0; toggle < 2; toggle++) {
                [peer toggleNotesList:self];
                [self nv_o2Layout]; [peer nv_o2Layout];
                [self nv_o2Completed:primaryQuery note:anchor identities:primaryIdentities visible:NO];
                [peer nv_o2Completed:peerQuery note:anchor identities:peerIdentities visible:NO];
                [peer nv_o2FocusPreserved];
                [peer toggleNotesList:self];
                [self nv_o2Layout]; [peer nv_o2Layout];
                [self nv_o2Completed:primaryQuery note:anchor identities:primaryIdentities visible:YES];
                [peer nv_o2Completed:peerQuery note:anchor identities:peerIdentities visible:YES];
                [peer nv_o2FocusPreserved];
                Check(NSEqualRects([window frame], primaryFrame) && NSEqualRects([[peer window] frame], peerFrame),
                    @"list collapse and restoration keep both window frames unchanged");
            }
            [peerIdentities release];
            [[peer window] close];
            Check([[app browserControllers] count] == 1 && [[[session textStorage] layoutManagers] count] == 1,
                @"native close unregisters the peer and detaches only its source layout manager");
            [pool drain];
            for (NSUInteger tick = 0; tick < 40; tick++) {
                NSAutoreleasePool *tickPool = [[NSAutoreleasePool alloc] init]; Pump(); [tickPool drain];
                if ([[NVSummaryDeaths objectForKey:@"controller"] unsignedIntegerValue] == cycle + 1 &&
                    [[NVSummaryDeaths objectForKey:@"status"] unsignedIntegerValue] == cycle + 1 &&
                    [[NVSummaryDeaths objectForKey:@"action"] unsignedIntegerValue] == cycle + 1) break;
            }
            for (NSString *name in @[@"controller", @"status", @"action"]) {
                Check([[NVSummaryDeaths objectForKey:name] unsignedIntegerValue] == cycle + 1,
                    [NSString stringWithFormat:@"native close releases exactly one %@", name]);
            }
            [self nv_o2Layout];
            [self nv_o2Completed:cycle ? @"" : @"anchor" note:anchor identities:primaryIdentities visible:YES];
            Check([library totalNoteCount] == 2 && [titleOfNote(anchor) isEqual:@"Anchor"] &&
                [[[anchor contentString] string] isEqual:@"anchor shared source"] &&
                [[[other contentString] string] isEqual:@"other source"], @"all lifecycle transitions preserve fixture notes and shared source");
        }
        [primaryIdentities release]; [anchor release]; [other release];
        [library flushAllNoteChanges]; [library closeJournal];
        NSLog(@"SUMMARY_OUSTERHOUT_ROUND2_PASS checks=%lu deaths=%@", (unsigned long)Checks, NVSummaryDeaths);
        exit(0);
    } @catch (NSException *exception) { NSLog(@"FAIL exception: %@\n%@", exception, [exception callStackSymbols]); exit(1); }
}
