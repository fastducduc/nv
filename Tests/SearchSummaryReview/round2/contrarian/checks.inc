- (void)nv_summaryRestoreAwait {
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:8];
    while ((![[self browserSession] searchResultsAreCurrent] || pendingSearchRestoration) &&
           [deadline timeIntervalSinceNow] > 0) Pump();
    Check([[self browserSession] searchResultsAreCurrent] && !pendingSearchRestoration &&
        ![[self browserSession] searchError], @"restored search finishes its real worker and pending selection restoration");
    Pump(); Pump(); // Include the queued divider-restoration layout.
}
- (void)nv_summaryRestoreCheckSurface {
    Check([searchStatusField isHidden] && [[searchStatusField stringValue] length] == 0,
        @"restored successful search leaves no visible or stored summary text");
    Check(NSEqualRects([notesScrollView frame], [notesSubview bounds]),
        @"relaunch restoration leaves no persistent blank summary strip");
    Check([[field toolTip] length] == 0 && [[searchStatusField toolTip] length] == 0,
        @"restoration clears both summary tooltip surfaces");
    Check(![prefsController showTitleInTopSection] && ![prefsController showTagsInTopSection] &&
        ![prefsController showBodyControlsInTopSection] && [noteTitleField isHidden] && [noteTagsField isHidden] &&
        [bodyModeControl isHidden], @"hidden header preferences and actual views agree across restart");
}
- (void)nv_runTests {
    @try {
        NSUInteger phase = (NSUInteger)atoi(getenv("NV_REVIEW_PHASE") ?: "1");
        NSString *path = [TestDirectory stringByAppendingPathComponent:@"saved-search-cases.plist"];
        NotationController *library = [[NVApplicationController sharedController] library];
        [self nv_activateBrowser];
        if (phase == 1) {
            NoteObject *road = MakeNote(library, @"Road map", @"road planning source");
            NoteObject *operators = MakeNote(library, @"a|b operator reference", @"literal a|b source");
            MakeNote(library, @"Plain", @"unrelated content");
            [prefsController setAutoCompleteSearches:NO sender:self];
            [prefsController setShowTitleInTopSection:NO sender:self];
            [prefsController setShowTagsInTopSection:NO sender:self];
            [prefsController setShowBodyControlsInTopSection:NO sender:self];
            NSArray *queries = @[@"road", @"a|b", @" \t\n  "];
            NSArray *expectedCounts = @[@2, @2, @3];
            NSArray *selectedNotes = @[road, operators];
            NSMutableArray *records = [NSMutableArray array];
            for (NSUInteger index = 0; index < [queries count]; index++) {
                [self searchForString:queries[index] mode:@"fuzzy"]; [self nv_summaryRestoreAwait];
                Check([[self browserSession] resultCount] == [expectedCounts[index] unsignedIntegerValue],
                    @"save fixture has the expected literal, duplicate, or whitespace result count");
                if (index < 2) {
                    NSUInteger row = NSNotFound;
                    NVBrowserSession *session = [self browserSession];
                    for (NSUInteger candidate = 0; candidate < [session resultCount]; candidate++) {
                        if ([session noteObjectAtFilteredIndex:candidate] == selectedNotes[index] &&
                            [[session matchKindAtIndex:candidate] isEqual:@"fuzzy"]) row = candidate;
                    }
                    Check(row != NSNotFound, @"save fixture locates the fuzzy occurrence rather than the title occurrence");
                    [notesTableView selectRowAndScroll:row]; Pump();
                } else {
                    [notesTableView deselectAll:self]; Pump();
                }
                [self setNotesListHeight:125 + index * 25]; Pump();
                [self nv_summaryRestoreCheckSurface];
                if (index == 0) {
                    // Capture a legacy child-view gap without adding a fictitious persistent state key.
                    NSRect frame = [notesScrollView frame]; frame.size.height -= 24; [notesScrollView setFrame:frame];
                }
                NSDictionary *state = [self browserWindowState];
                Check([state[@"search"] isEqual:queries[index]] && [state[@"searchMode"] isEqual:@"fuzzy"],
                    @"saved browser state preserves the exact query and mode");
                if (index == 0) Check(NSHeight([notesSubview bounds]) - NSHeight([notesScrollView frame]) == 24,
                    @"the first saved state was captured with the legacy 24-point child-view gap present");
                [records addObject:@{@"state": state, @"query": queries[index], @"rowCount": expectedCounts[index],
                    @"hasTerms": @(index < 2), @"selectedRowKey": [self selectedSearchResultRowKey] ?: @"",
                    @"listHeight": @(NSHeight([notesSubview bounds]))}];
                [self updateSearchAffordance];
            }
            Check([records writeToFile:path atomically:YES], @"three actual browser states are written for a second app process");
            [library flushAllNoteChanges];
            [[NSUserDefaults standardUserDefaults] synchronize];
            NSLog(@"SUMMARY_RESTORE_PHASE_PASS phase=1 checks=%lu", (unsigned long)Checks);
        } else {
            NSArray *records = [NSArray arrayWithContentsOfFile:path];
            Check([records count] == 3 && [library totalNoteCount] == 3,
                @"the second process reopens the saved states and the same three-note library");
            for (NSUInteger index = 0; index < [records count]; index++) {
                NSDictionary *record = records[index];
                NSDictionary *state = record[@"state"];
                [self restoreBrowserWindowState:state]; [self nv_summaryRestoreAwait];
                NVBrowserSession *session = [self browserSession];
                Check([[session searchString] isEqual:record[@"query"]] && [[field stringValue] isEqual:record[@"query"]] &&
                    [[self searchMode] isEqual:@"fuzzy"], @"relaunch restores each exact query in its original fuzzy mode");
                Check([session resultCount] == [record[@"rowCount"] unsignedIntegerValue] &&
                    [session hasSearchTerms] == [record[@"hasTerms"] boolValue],
                    @"relaunch preserves native literal-operator and whitespace query semantics");
                Check(state[@"note"] ? [[self selectedSearchResultRowKey] ?: @"" isEqual:record[@"selectedRowKey"]] :
                    [self selectedNoteObject] == nil,
                    @"relaunch retains the selected fuzzy occurrence or deliberate empty note selection");
                NSDictionary *after = [self browserWindowState];
                Check([(after[@"note"] ?: @"") isEqual:(state[@"note"] ?: @"")],
                    @"restoration keeps note identity across the process boundary");
                Check(fabs(NSHeight([notesSubview bounds]) - [record[@"listHeight"] doubleValue]) < 1.0,
                    @"the stored divider height is restored without absorbing a child-view summary gap");
                if (index == 0 && getenv("NV_SUMMARY_RESTORE_NEGATIVE")) {
                    NSRect frame = [notesScrollView frame]; frame.size.height -= 24; [notesScrollView setFrame:frame];
                }
                [self nv_summaryRestoreCheckSurface];
                Check([createNoteButton isHidden], @"restored matching and whitespace searches do not offer spurious note creation");
                NSLog(@"RESTORED_CASE index=%lu queryLength=%lu rows=%lu selected=%d listHeight=%.1f parentHeight=%.1f",
                    (unsigned long)index, (unsigned long)[[field stringValue] length], (unsigned long)[session resultCount],
                    [self selectedNoteObject] != nil, NSHeight([notesScrollView frame]), NSHeight([notesSubview bounds]));
            }
            NSLog(@"SUMMARY_RESTORE_PHASE_PASS phase=2 checks=%lu", (unsigned long)Checks);
        }
        [self performSelector:@selector(nv_finishTests) withObject:nil afterDelay:0.2];
    } @catch (NSException *exception) { NSLog(@"SUMMARY_RESTORE_EXCEPTION %@", exception); exit(2); }
}
