#import <Cocoa/Cocoa.h>
#import "NVSearchService.h"

// Hold genuine service completions without changing scoring or the browser session.
@interface NVSummaryCompatibilityDelivery : NSObject {
@public NVSearchCompletion completion; NVSearchResult *result; NSError *error;
}
- (void)deliver;
- (void)failWithDescription:(NSString *)description;
@end
@implementation NVSummaryCompatibilityDelivery
- (void)deliver { completion(result, error); }
- (void)failWithDescription:(NSString *)description {
    completion(nil, [NSError errorWithDomain:@"SummaryCompatibilityReview" code:1
        userInfo:@{NSLocalizedDescriptionKey:description}]);
}
- (void)dealloc { [completion release]; [result release]; [error release]; [super dealloc]; }
@end
static NSMutableArray *NVSummaryCompatibilityDeliveries;
static NSUInteger NVSummaryCompatibilityRequests;
