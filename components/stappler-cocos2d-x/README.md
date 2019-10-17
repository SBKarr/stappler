<img src="http://www.cocos2d-x.org/attachments/801/cocos2dx_portrait.png" width=200>

Original source url for [cocos2d-x][1] at [https://github.com/cocos2d/cocos2d-x][2]

cocos2d-x
=========

[cocos2d-x][1] is a multi-platform framework for building 2d games, interactive books, demos and other graphical applications.
It is based on [cocos2d-iphone][3], but instead of using Objective-C, it uses C++.
It works on iOS, Android, Tizen, Windows Phone and Store Apps, OS X, Windows, Linux and Web platforms.

cocos2d-x is:

  * Fast
  * Free
  * Easy to use
  * Community supported


stappler
========

[stappler][4] - application toolkit for linux/win32/ios/android.
It can be used to create command-line, GUI or server applications with common API.

GUI applications based on cocos2d-x framework, with widgets, inspired by Google Material Design.

Server applications (Serenity project) is a modules for Apache HTTPD server, with PostgreSQL as DBMS.

With Stappler toolkit you can create a whole set of applications for your mobile/web project: native mobile applications, webservices and supporting command-line tools.


Modifications
=============

This version of cocos2d-x is not compatible with original cocos2d-x release. Some modifications was made to improve performance and usability in stappler project.

1. Improved `cocos2d::Node` callbacks: `onContentSizeDirty`, `onTransformDirty`, `onReorderChildDirty`

2. Tag for `cocos2d::Action` has `uint32_t` type instead of `int` to match `_hash32` prefix in stappler

3. Rewritten `cocos2d::Component` to catch new callbacks and `visit` call

4. `cocos2d::Renderer` splitted to `cocos2d::Renderer` and `cocos2d::RendererView` for stappler semi-auto batching algorithm

5. Added `zPath` concept for render commands for stappler semi-auto batching

6. Added `cocos2d::Director::Projection::_EUCLID` for pixel-perfect drawing

7. Rewritten default desktop views to support mutli-touch emulation and resizing

8. Added `stride` parameter for `cocos2d::Texture2D` init and update

9. Removed subsystems, that implemented in stappler (like labels)



[1]: http://www.cocos2d-x.org "cocos2d-x"
[2]: https://github.com/cocos2d/cocos2d-x "cocos2d-x on github"
[3]: http://www.cocos2d-iphone.org "cocos2d for iPhone"
[4]: https://github.com/SBKarr/stappler "stappler on github"
