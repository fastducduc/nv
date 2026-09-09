- (NVSummaryCompatibilityDelivery *)nv_compatibilityAwait:(NSUInteger)index {
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:8];
    while ([NVSummaryCompatibilityDeliveries count] <= index && [deadline timeIntervalSinceNow] > 0) Pump();
    Check([NVSummaryCompatibilityDeliveries count] > index, @"native search reaches the controlled delivery boundary");
    return [NVSummaryCompatibilityDeliveries objectAtIndex:index];
}
- (void)nv_compatibilityCheckAction:(SEL)action label:(NSString *)label {
    Check(![createNoteButton isHiddenOrHasHiddenAncestor] && [createNoteButton isEnabled],
        [label stringByAppendingString:@": action remains visible and enabled"]);
    Check([createNoteButton target] == self && [createNoteButton action] == action && [self respondsToSelector:action],
        [label stringByAppendingString:@": native target and selector are intact"]);
    Check(NSContainsRect([notesSubview bounds], [createNoteButton frame]),
        [label stringByAppendingString:@": button stays inside the minimum-size list container"]);
    NSRect bounds = [createNoteButton bounds];
    CGFloat xs[] = {NSMidX(bounds), NSMinX(bounds) + 16, NSMaxX(bounds) - 16};
    for (NSUInteger i = 0; i < 3; ++i) {
        NSPoint point = [createNoteButton convertPoint:NSMakePoint(xs[i], NSMidY(bounds)) toView:[notesSubview superview]];
        NSView *hit = [notesSubview hitTest:point];
        Check(hit == createNoteButton || [hit isDescendantOf:createNoteButton],
            [NSString stringWithFormat:@"%@: native hit point %lu reaches the action", label, (unsigned long)i]);
    }
}
- (void)nv_compatibilityCheckCompleted {
    Check([[self browserSession] searchResultsAreCurrent] && ![[self browserSession] searchError],
        @"the actual session publishes a current successful completion");
    Check([searchStatusField isHidden] && ![[searchStatusField stringValue] length] &&
          ![[searchStatusField toolTip] length] && ![[field toolTip] length],
        @"completion clears the long error from the label and both tooltips");
    Check(NSEqualRects([notesScrollView frame], [notesSubview bounds]),
        @"completion restores the full minimum-size list area");
}
- (void)nv_runTests {
    @try {
        [self nv_activateBrowser];
        NotationController *library = [[NVApplicationController sharedController] library];
        NoteObject *note = MakeNote(library, @"Road map", @"Original compatibility fixture source");
        [prefsController setAutoCompleteSearches:NO sender:self];
        [window setContentSize:[window contentMinSize]];
        [[window contentView] layoutSubtreeIfNeeded]; Pump();
        Check(NSWidth([notesSubview bounds]) == 480 && NSHeight([notesSubview bounds]) >= 96 &&
              NSHeight([notesSubview bounds]) < 130, @"fixture uses the supported minimum-width window and a short list");
        NSLog(@"COMPATIBILITY_GEOMETRY content=%@ list=%@ button=%@ status=%@",
            NSStringFromRect([[window contentView] bounds]), NSStringFromRect([notesSubview bounds]),
            NSStringFromRect([createNoteButton frame]), NSStringFromRect([searchStatusField frame]));
        NSButton *originalButton = createNoteButton;
        NSTextField *originalStatus = searchStatusField;
        NVBrowserSession *session = [self browserSession];
        NSString *query = @"unmatched compatibility action fixture";
        NSString *longError = @"Die Suche konnte wegen einer vorübergehenden Unterbrechung nicht abgeschlossen werden. Bitte versuchen Sie die Suche erneut. — 検索を完了できませんでした。もう一度検索してください。 — La recherche ne peut pas se terminer pour le moment. Réessayez la recherche.";

        NVSummaryCompatibilityDeliveries = [NSMutableArray new];
        Method requestMethod = class_getInstanceMethod([NVSearchService class], @selector(requestForOwner:query:completion:));
        IMP originalRequest = method_getImplementation(requestMethod);
        IMP gate = imp_implementationWithBlock(^NSUInteger(NVSearchService *service, id owner, NSString *requestQuery, NVSearchCompletion completion) {
            ++NVSummaryCompatibilityRequests;
            return ((NSUInteger(*)(id,SEL,id,id,id))originalRequest)(service, @selector(requestForOwner:query:completion:), owner, requestQuery,
                ^(NVSearchResult *result, NSError *error) {
                    NVSummaryCompatibilityDelivery *delivery = [[[NVSummaryCompatibilityDelivery alloc] init] autorelease];
                    delivery->completion = [completion copy]; delivery->result = [result retain]; delivery->error = [error retain];
                    [NVSummaryCompatibilityDeliveries addObject:delivery];
                });
        });
        method_setImplementation(requestMethod, gate);
        [self searchForString:query mode:@"fuzzy"];
        [[self nv_compatibilityAwait:0] failWithDescription:longError];
        Check(![session searchResultsAreCurrent] && [session searchError] != nil && ![session searchPending],
            @"the production session accepts the current failed result");
        Check(![searchStatusField isHidden] && [[searchStatusField stringValue] isEqual:longError],
            @"the long multilingual error remains the complete status value");
        Check([[searchStatusField toolTip] isEqual:longError] && [[field toolTip] isEqual:longError],
            @"both native tooltips preserve the entire long error");
        Check([longError sizeWithAttributes:@{NSFontAttributeName:[searchStatusField font]}].width > NSWidth([searchStatusField bounds]) &&
              [[searchStatusField cell] lineBreakMode] == NSLineBreakByTruncatingTail,
            @"the error exceeds the native label width and uses tail truncation");
        Check(NSContainsRect([notesSubview bounds], [searchStatusField frame]) &&
              !NSIntersectsRect([searchStatusField frame], [createNoteButton frame]),
            @"the truncated error label stays inside the list without overlapping Retry");
        NSRect errorList = [notesSubview bounds]; errorList.size.height -= 24;
        Check(NSEqualRects([notesScrollView frame], errorList), @"error status reserves exactly one 24-point strip");
        Check([[createNoteButton title] isEqual:NSLocalizedString(@"Retry Search", nil)], @"the actual button displays Retry Search");
        if (getenv("NV_SUMMARY_COMPATIBILITY_NEGATIVE"))
            [notesSubview addSubview:notesScrollView positioned:NSWindowAbove relativeTo:createNoteButton];
        [self nv_compatibilityCheckAction:@selector(retrySearch:) label:@"Retry"];

        NSUInteger requests = NVSummaryCompatibilityRequests;
        [createNoteButton performClick:self];
        Check(NVSummaryCompatibilityRequests == requests + 1 && [session searchPending] && ![session searchError],
            @"the native Retry action issues exactly one new request and clears the error");
        Check([[session searchString] isEqual:query] && [[field stringValue] isEqual:query] && [library totalNoteCount] == 1,
            @"Retry preserves the query and does not create a note");
        [[self nv_compatibilityAwait:1] deliver];
        [self nv_compatibilityCheckCompleted];
        Check([session resultCount] == 0 && createNoteButton == originalButton && searchStatusField == originalStatus,
            @"zero-result completion reuses the original native action and status controls");
        [self nv_compatibilityCheckAction:@selector(createNoteFromSearch:) label:@"Create after Retry"];

        [self searchForString:@"Road" mode:@"exact"];
        [self nv_compatibilityCheckCompleted];
        Check([session resultCount] == 1 && [createNoteButton isHidden], @"switching to a matching Exact query hides the action");
        [self searchForString:query mode:@"fuzzy"];
        [[self nv_compatibilityAwait:2] deliver];
        [self nv_compatibilityCheckCompleted];
        [self nv_compatibilityCheckAction:@selector(createNoteFromSearch:) label:@"Create after mode switch"];
        Check(createNoteButton == originalButton && searchStatusField == originalStatus &&
              [library totalNoteCount] == 1 && [[[note contentString] string] isEqual:@"Original compatibility fixture source"],
            @"mode switching retains the original controls, note count, and source characters");

        method_setImplementation(requestMethod, originalRequest); imp_removeBlock(gate);
        [NVSummaryCompatibilityDeliveries removeAllObjects]; [NVSummaryCompatibilityDeliveries release]; NVSummaryCompatibilityDeliveries = nil;
        [library flushAllNoteChanges]; [library closeJournal];
        NSLog(@"TORVALDS_SUMMARY_R2_PASS checks=%lu requests=%lu", (unsigned long)Checks, (unsigned long)NVSummaryCompatibilityRequests);
        exit(0);
    } @catch (NSException *exception) { NSLog(@"FAIL exception: %@", exception); exit(2); }
}
