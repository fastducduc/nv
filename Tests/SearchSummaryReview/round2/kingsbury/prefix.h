#import <Cocoa/Cocoa.h>
#import "NVSearchService.h"
#import "AppController.h"

@interface AppController (SummaryOwnerReviewPrivate)
- (void)showSearchProgress;
@end

// Supplied by the event history, independently of the observed UI.
typedef struct {
    NSString *query, *mode, *title, *status;
    BOOL current, retry;
} OwnerState;

@interface OwnerDelivery : NSObject {
@public NVSearchCompletion completion; NVSearchResult *result; NSError *error;
    void *ownerKey;
}
- (void)deliver;
- (void)fail;
@end
@implementation OwnerDelivery
- (void)deliver { completion(result,error); }
- (void)fail { completion(nil,[NSError errorWithDomain:@"OwnerReview" code:1
    userInfo:@{NSLocalizedDescriptionKey:@"Controlled owner failure"}]); }
- (void)dealloc { [completion release]; [result release]; [error release]; [super dealloc]; }
@end
static NSMutableArray *OwnerDeliveries;
