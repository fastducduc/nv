- (void)nv_oCheck:(OwnerState)state {
    Check([[field stringValue] isEqual:state.query] && [[[self browserSession] searchString] isEqual:state.query],
        @"owner-local visible and committed query follow the independent model");
    Check([[self searchMode] isEqual:state.mode] && [[self browserSession] searchResultsAreCurrent] == state.current,
        @"owner-local mode and currency follow the independent model");
    Check([searchStatusField isHidden] == ![state.status length], @"owner-local status visibility follows the independent model");
    Check([[searchStatusField stringValue] isEqual:state.status], @"owner-local status text follows the independent model");
    Check([[field toolTip] isEqual:state.status] && [[searchStatusField toolTip] isEqual:state.status],
        @"owner-local tooltips follow the independent model");
    NSRect expected=[notesSubview bounds];
    if ([state.status length]) expected.size.height=MAX(0,expected.size.height-24);
    Check(NSEqualRects([notesScrollView frame],expected), @"owner-local list allocation follows the independent model");
    Check([createNoteButton isHidden] != state.retry, @"only the modeled owner's Retry action is visible");
    if (state.retry) Check([createNoteButton action]==@selector(retrySearch:), @"error action dispatches Retry Search in its owner");
    Check([titleOfNote([self selectedNoteObject]) isEqual:state.title], @"owner-local document selection follows the independent model");
}
- (OwnerDelivery *)nv_oAwait:(NSUInteger)index {
    NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:10];
    while ([OwnerDeliveries count]<=index && [deadline timeIntervalSinceNow]>0) Pump();
    Check([OwnerDeliveries count]>index, @"real native computation reaches the controlled completion boundary");
    OwnerDelivery *delivery=[OwnerDeliveries objectAtIndex:index];
    Check(delivery->ownerKey==[self browserSession], @"captured native completion belongs to the submitting browser session");
    return delivery;
}
- (OwnerDelivery *)nv_oPending:(NSString *)query reveal:(NoteObject *)note {
    NSUInteger index=[OwnerDeliveries count];
    [self searchForString:query mode:@"fuzzy"]; [self revealNote:note options:0];
    return [self nv_oAwait:index];
}
- (void)nv_oProgress {
    NSDate *deadline=[NSDate dateWithTimeIntervalSinceNow:3];
    while (![searchStatusField stringValue].length && [deadline timeIntervalSinceNow]>0) Pump();
    Check([[self browserSession] searchPending] && searchStatusDelayElapsed, @"owner's real delayed-progress timer fires with its request still pending");
}
- (OwnerDelivery *)nv_oRetry:(NoteObject *)note {
    NSUInteger index=[OwnerDeliveries count];
    [createNoteButton performClick:self]; [self revealNote:note options:0];
    return [self nv_oAwait:index];
}
- (void)nv_runTests {
    @try {
        NVApplicationController *app=[NVApplicationController sharedController];
        NotationController *library=[app library];
        NoteObject *road=MakeNote(library,@"Road map",@"road source");
        NoteObject *copper=MakeNote(library,@"Copper",@"road copper source");
        MakeNote(library,@"Empty",@"miscellaneous source");
        [prefsController setAutoCompleteSearches:NO sender:self];
        [self searchForString:@"road" mode:@"exact"]; [self revealNote:road options:0];
        [app newWindow:self]; Pump();
        AppController *peer=[[[app browserControllers] lastObject] retain];
        Check(peer!=self && [app browserControllers].count==2 && [peer browserSession]!=[self browserSession],
            @"two actual browsers have independent sessions over the fixture library");
        [peer searchForString:@"copper" mode:@"exact"]; [peer revealNote:copper options:0];
        [self nv_oCheck:(OwnerState){@"road",@"exact",@"Road map",@"",YES,NO}];
        [peer nv_oCheck:(OwnerState){@"copper",@"exact",@"Copper",@"",YES,NO}];

        OwnerDeliveries=[NSMutableArray new];
        Method request=class_getInstanceMethod([NVSearchService class],@selector(requestForOwner:query:completion:));
        IMP original=method_getImplementation(request);
        IMP gate=imp_implementationWithBlock(^NSUInteger(NVSearchService *service,id owner,NSString *query,NVSearchCompletion completion) {
            void *key=owner;
            return ((NSUInteger(*)(id,SEL,id,id,id))original)(service,@selector(requestForOwner:query:completion:),owner,query,
                ^(NVSearchResult *result,NSError *error) {
                    OwnerDelivery *delivery=[[[OwnerDelivery alloc]init]autorelease];
                    delivery->completion=[completion copy]; delivery->result=[result retain]; delivery->error=[error retain]; delivery->ownerKey=key;
                    [OwnerDeliveries addObject:delivery];
                });
        });
        method_setImplementation(request,gate);
        NSString *searching=NSLocalizedString(@"Searching…",nil);

        NSLog(@"HISTORY A: overlapping owners, first owner error, second owner success");
        OwnerDelivery *a=[self nv_oPending:@"road" reveal:road];
        OwnerDelivery *b=[peer nv_oPending:@"copper" reveal:copper];
        [self nv_oProgress]; [peer nv_oProgress];
        [self nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Road map",searching,NO,NO}];
        [peer nv_oCheck:(OwnerState){@"copper",@"fuzzy",@"Copper",searching,NO,NO}];
        [a fail];
        [self nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Road map",@"Controlled owner failure",NO,YES}];
        [peer nv_oCheck:(OwnerState){@"copper",@"fuzzy",@"Copper",searching,NO,NO}];
        if (getenv("NV_SUMMARY_WRONG_OWNER")) {
            Method affordance=class_getInstanceMethod([AppController class],@selector(updateSearchAffordance));
            IMP normal=method_getImplementation(affordance);
            AppController *victim=self;
            method_setImplementation(affordance,imp_implementationWithBlock(^(AppController *browser) {
                ((void(*)(id,SEL))normal)(browser,@selector(updateSearchAffordance));
                if (browser==peer && [[browser browserSession] searchResultsAreCurrent]) {
                    NSTextField *label=[victim valueForKey:@"searchStatusField"];
                    [label setHidden:YES]; [label setStringValue:@""]; [label setToolTip:@""];
                    [[victim valueForKey:@"field"] setToolTip:@""];
                    [[victim valueForKey:@"notesScrollView"] setFrame:[[victim valueForKey:@"notesSubview"] bounds]];
                }
            }));
        }
        [b deliver];
        [self nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Road map",@"Controlled owner failure",NO,YES}];
        [peer nv_oCheck:(OwnerState){@"copper",@"fuzzy",@"Copper",@"",YES,NO}];

        NSLog(@"HISTORY B: first owner's Retry overlaps a new peer query; publish in reverse submission order");
        OwnerDelivery *retry=[self nv_oRetry:road];
        OwnerDelivery *next=[peer nv_oPending:@"road" reveal:copper];
        [self nv_oProgress]; [peer nv_oProgress];
        [a fail]; [a deliver]; [b deliver];
        [self nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Road map",searching,NO,NO}];
        [peer nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Copper",searching,NO,NO}];
        [next deliver];
        [self nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Road map",searching,NO,NO}];
        [peer nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Copper",@"",YES,NO}];
        [retry deliver]; [a fail]; [peer showSearchProgress];
        [self nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Road map",@"",YES,NO}];
        [peer nv_oCheck:(OwnerState){@"road",@"fuzzy",@"Copper",@"",YES,NO}];

        NSLog(@"HISTORY C: close a peer with delayed progress; finish survivor, then replay closed-owner callbacks");
        OwnerDelivery *survivor=[self nv_oPending:@"copper" reveal:copper];
        OwnerDelivery *closed=[peer nv_oPending:@"road close" reveal:road];
        [self nv_oProgress]; [peer nv_oProgress];
        [self nv_oCheck:(OwnerState){@"copper",@"fuzzy",@"Road map",searching,NO,NO}];
        [peer nv_oCheck:(OwnerState){@"road close",@"fuzzy",@"Copper",searching,NO,NO}];
        [[peer window] performClose:self]; Pump();
        Check([app browserControllers].count==1 && ![[app browserControllers] containsObject:peer] && ![[peer window] isVisible],
            @"actual close removes only the pending peer browser");
        [self nv_oCheck:(OwnerState){@"copper",@"fuzzy",@"Road map",searching,NO,NO}];
        [survivor deliver];
        [self nv_oCheck:(OwnerState){@"copper",@"fuzzy",@"Copper",@"",YES,NO}];
        [closed fail]; [closed deliver]; [b fail]; [next deliver]; Pump(); Pump(); Pump();
        [self nv_oCheck:(OwnerState){@"copper",@"fuzzy",@"Copper",@"",YES,NO}];
        Check([app browserControllers].count==1 && [[app browserControllers] objectAtIndex:0]==self,
            @"closed-owner stale callbacks do not reopen a browser or replace the survivor");
        Check([library totalNoteCount]==3, @"interleaved status, retry, and close histories preserve three fixture notes");
        Check([OwnerDeliveries count]==6, @"exactly six intended native search deliveries cover all three histories");
        method_setImplementation(request,original); imp_removeBlock(gate);
        [OwnerDeliveries removeAllObjects]; [OwnerDeliveries release]; OwnerDeliveries=nil; [peer release];
        [library flushAllNoteChanges]; [library closeJournal];
        NSLog(@"SEARCH_SUMMARY_KINGSBURY_R2_PASS checks=%lu",(unsigned long)Checks); exit(0);
    } @catch (NSException *exception) { NSLog(@"FAIL exception: %@\n%@",exception,[exception callStackSymbols]); exit(1); }
}
