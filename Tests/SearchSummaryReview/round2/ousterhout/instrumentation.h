#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

// These witnesses retain names only, never the observed controller or control.
static NSMutableDictionary *NVSummaryDeaths;
static char NVSummaryWitnessKey;
@interface NVSummaryWitness : NSObject { NSString *name; }
- (id)initWithName:(NSString *)value;
@end
@implementation NVSummaryWitness
- (id)initWithName:(NSString *)value { if ((self = [super init])) name = [value copy]; return self; }
- (void)dealloc {
    [NVSummaryDeaths setObject:@([[NVSummaryDeaths objectForKey:name] unsignedIntegerValue] + 1) forKey:name];
    [name release]; [super dealloc];
}
@end
static void NVSummaryObserve(id object, NSString *name) {
    NVSummaryWitness *witness = [[NVSummaryWitness alloc] initWithName:name];
    objc_setAssociatedObject(object, &NVSummaryWitnessKey, witness, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [witness release];
}
