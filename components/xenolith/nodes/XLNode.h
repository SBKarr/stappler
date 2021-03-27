/**
Copyright (c) 2021 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#ifndef COMPONENTS_XENOLITH_NODES_XLNODE_H_
#define COMPONENTS_XENOLITH_NODES_XLNODE_H_

#include "XLRenderFrameInfo.h"

namespace stappler::xenolith {

enum class NodeFlags {
	None,
	TransformDirty = 1 << 0,
	ContentSizeDirty = 1 << 1,

	DirtyMask = TransformDirty | ContentSizeDirty
};

SP_DEFINE_ENUM_AS_MASK(NodeFlags)

class Node : public Ref {
public:
	static constexpr uint32_t INVALID_TAG = maxOf<uint32_t>();

	Node();
	virtual ~Node();

	virtual bool init();

	virtual void setLocalZOrder(int32_t localZOrder);
	virtual int32_t getLocalZOrder() const { return _zOrder; }

	virtual void setScale(float scale);
	virtual void setScale(const Vec2 &);
	virtual void setScale(const Vec3 &);
	virtual void setScaleX(float scaleX);
	virtual void setScaleY(float scaleY);
	virtual void setScaleZ(float scaleZ);

	virtual const Vec3 & getScale() const { return _scale; }

	virtual void setPosition(const Vec2 &position);
	virtual void setPosition(const Vec3 &position);
	virtual void setPositionX(float);
	virtual void setPositionY(float);
	virtual void setPositionZ(float);

	virtual const Vec3 & getPosition() const { return _position; }

	virtual void setSkewX(float skewX);
	virtual void setSkewY(float skewY);

	virtual const Vec2 & getSkew() const { return _skew; }

	/**
	 * Sets the anchor point in percent.
	 *
	 * anchorPoint is the point around which all transformations and positioning manipulations take place.
	 * It's like a pin in the node where it is "attached" to its parent.
	 * The anchorPoint is normalized, like a percentage. (0,0) means the bottom-left corner and (1,1) means the top-right corner.
	 * But you can use values higher than (1,1) and lower than (0,0) too.
	 * The default anchorPoint is (0.5,0.5), so it starts in the center of the node.
	 * @note If node has a physics body, the anchor must be in the middle, you cann't change this to other value.
	 *
	 * @param anchorPoint   The anchor point of node.
	 */
	virtual void setAnchorPoint(const Vec2& anchorPoint);
	virtual const Vec2 & getAnchorPoint() const { return _anchorPoint; }

	/**
	 * Sets the untransformed size of the node.
	 *
	 * The contentSize remains the same no matter the node is scaled or rotated.
	 * All nodes has a size. Layer and Scene has the same size of the screen.
	 *
	 * @param contentSize   The untransformed size of the node.
	 */
	virtual void setContentSize(const Size& contentSize);
	virtual const Size& getContentSize() const { return _contentSize; }

	virtual void setVisible(bool visible);
	virtual bool isVisible() const { return _visible; }

	virtual void setRotation(float rotation);
	virtual void setRotation(const Vec3 & rotation);
	virtual void setRotation(const Quaternion & quat);

	virtual float getRotation() const { return _rotation.z; }
	virtual const Vec3 &getRotation3D() const { return _rotation; }
	virtual const Quaternion &getRotationQuat() const { return _rotationQuat; }

	template<typename N, typename ... Args>
	auto addChild(N *child, Args &&... args) -> N * {
		addChildNode(child, std::forward<Args>(args)...);
		return child;
	}

	template<typename N, typename ... Args>
	auto addChild(const Rc<N> &child, Args &&... args) -> N * {
		addChildNode(child.get(), std::forward<Args>(args)...);
		return child.get();
	}

	virtual void addChildNode(Node *child);
	virtual void addChildNode(Node *child, int32_t localZOrder);
	virtual void addChildNode(Node *child, int32_t localZOrder, uint32_t tag);

	virtual Node* getChildByTag(uint32_t tag) const;

	virtual const Vector<Rc<Node>>& getChildren() const { return _children; }
	virtual ssize_t getChildrenCount() const { return _children.size(); }

	virtual void setParent(Node *parent);
	virtual Node* getParent() const { return _parent; }

	virtual void removeFromParent(bool cleanup = true);
	virtual void removeChild(Node *child, bool cleanup = true);
	virtual void removeChildByTag(uint32_t tag, bool cleanup = true);
	virtual void removeAllChildren(bool cleanup = true);

	virtual void reorderChild(Node *child, int32_t localZOrder);

	/**
	 * Sorts the children array once before drawing, instead of every time when a child is added or reordered.
	 * This appraoch can improves the performance massively.
	 * @note Don't call this manually unless a child added needs to be removed in the same frame.
	 */
	virtual void sortAllChildren();

	virtual uint32_t getTag() const { return _tag; }
	virtual void setTag(uint32_t tag);

	virtual bool isRunning() const { return _running; }

	virtual void onEnter();
	virtual void onExit();

	virtual void onContentSizeDirty();
	virtual void onTransformDirty();
	virtual void onReorderChildDirty();

	virtual void cleanup();

	/**
	 * Returns an AABB (axis-aligned bounding-box) in its parent's coordinate system.
	 *
	 * @return An AABB (axis-aligned bounding-box) in its parent's coordinate system
	 */
	virtual Rect getBoundingBox() const;

	virtual void resume();
	virtual void pause();

	virtual void updateChildrenTransform();
	virtual const Mat4& getNodeToParentTransform() const;

	virtual void setNodeToParentTransform(const Mat4& transform);
	virtual const Mat4& getParentToNodeTransform() const;

	virtual Mat4 getNodeToWorldTransform() const;
	virtual Mat4 getWorldToNodeTransform() const;

	Vec2 convertToNodeSpace(const Vec2& worldPoint) const;
	Vec2 convertToWorldSpace(const Vec2& nodePoint) const;
	Vec2 convertToNodeSpaceAR(const Vec2& worldPoint) const;
	Vec2 convertToWorldSpaceAR(const Vec2& nodePoint) const;

	virtual bool isCascadeOpacityEnabled() const { return _cascadeOpacityEnabled; }
	virtual bool isCascadeColorEnabled() const { return _cascadeColorEnabled; }

	virtual void setCascadeOpacityEnabled(bool cascadeOpacityEnabled);
	virtual void setCascadeColorEnabled(bool cascadeColorEnabled);

	virtual float getOpacity() const { return _realColor.a; }
	virtual float getDisplayedOpacity() const { return _displayedColor.a; }
	virtual void setOpacity(float opacity);
	virtual void updateDisplayedOpacity(float parentOpacity);

	virtual Color4F getColor() const { return _realColor; }
	virtual Color4F getDisplayedColor() const { return _displayedColor; }
	virtual void setColor(const Color4F& color, bool withOpacity = false);
	virtual void updateDisplayedColor(const Color4F& parentColor);

	virtual void setOpacityModifyRGB(bool value) { }
	virtual bool isOpacityModifyRGB() const { return false; };

    virtual void draw(RenderFrameInfo &, NodeFlags flags);
    virtual void visit(RenderFrameInfo &, NodeFlags parentFlags);

protected:
	virtual void updateCascadeOpacity();
	virtual void disableCascadeOpacity();
	virtual void updateCascadeColor();
	virtual void disableCascadeColor();
	virtual void updateColor() { }

    Mat4 transform(const Mat4 &parentTransform);
	NodeFlags processParentFlags(RenderFrameInfo &info, NodeFlags parentFlags);

	bool _is3d = false;
	bool _running = false;
	bool _visible = false;

	bool _cascadeColorEnabled = false;
	bool _cascadeOpacityEnabled = true;

	bool _contentSizeDirty = true;
	bool _reorderChildDirty = true;
	mutable bool _transformCacheDirty = true; // dynamic value
	mutable bool _transformInverseDirty = true; // dynamic value
	bool _transformDirty = true;

	uint32_t _tag = INVALID_TAG;
	int32_t _zOrder = 0;

	Vec2 _skew;
	Vec2 _anchorPoint;
	Size _contentSize;

	Vec3 _position;
	Vec3 _scale = Vec3(1.0f, 1.0f, 1.0f);
	Vec3 _rotation;

	// to support HDR, we use float colors;
	Color4F _displayedColor;
	Color4F _realColor;

	Quaternion _rotationQuat;

	mutable Mat4 _transform = Mat4::IDENTITY;
	mutable Mat4 _inverse = Mat4::IDENTITY;
	Mat4 _modelViewTransform = Mat4::IDENTITY;

	Vector<Rc<Node>> _children;
	Node *_parent = nullptr; // weak

	Function<void()> _onEnterCallback;
	Function<void()> _onExitCallback;
	Function<void()> _onContentSizeDirtyCallback;
	Function<void()> _onTransformDirtyCallback;
	Function<void()> _onReorderChildDirtyCallback;

	String _name;
};

}

#endif /* COMPONENTS_XENOLITH_NODES_XLNODE_H_ */
