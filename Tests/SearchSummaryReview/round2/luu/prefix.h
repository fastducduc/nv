#import <Cocoa/Cocoa.h>
#import "NVSearchService.h"

// Retain an actual search result at the service callback boundary. Scoring and
// the browser's pending notifications and progress timer remain production code.
@interface ViewportDelivery : NSObject {
@public NVSearchCompletion completion; NVSearchResult *result; NSError *error;
}
- (void)deliver;
@end
@implementation ViewportDelivery
- (void)deliver { completion(result, error); }
- (void)dealloc { [completion release]; [result release]; [error release]; [super dealloc]; }
@end
static NSMutableArray *ViewportDeliveries;
static NSMutableArray *ViewportMeasurements;
static BOOL ViewportExpectReclaimedSpace;
