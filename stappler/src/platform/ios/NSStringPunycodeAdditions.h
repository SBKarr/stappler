//
//  NSStringPunycodeAdditions.h
//  Punycode
//
//  Created by Wevah on 2005.11.02.
//  Copyright 2005-2012 Derailer. All rights reserved.
//
//  Distributed under an MIT-style license; please
//  see the included LICENSE file for details.
//
// From SBKarr: this allow us to open native-encoded urls (like cyrillic)

#import <Foundation/Foundation.h>

@interface PunycodeAddress : NSObject 

+ (NSString *)punycodeEncodedString:(NSString *)str;
+ (NSString *)punycodeDecodedString:(NSString *)str;

+ (NSString *)IDNAEncodedString:(NSString *)str;
+ (NSString *)IDNADecodedString:(NSString *)str;

+ (NSString *)decodedString:(NSString *)str;
+ (NSString *)encodedString:(NSString *)str;

+ (NSURL *)URLWithUnicodeString:(NSString *)URLString;
+ (NSString *)decodedURLString:(NSURL *)url;

@end
