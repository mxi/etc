#import <Foundation/Foundation.h>

/* application-supplied entry point. */
extern void mg_main(void);

int main(void)
{
    NSBundle *bundle = [NSBundle mainBundle];

    if (bundle)
    {
        NSLog(@"Bundle path: %@", [bundle bundlePath]);
        NSLog(@"Executable path: %@", [bundle executablePath]);
        NSLog(@"Resource path: %@", [bundle resourcePath]);
    }

    mg_main();

    return 0;
}