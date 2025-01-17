// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_BUBBLE_TRAY_BUBBLE_VIEW_H_
#define UI_VIEWS_BUBBLE_TRAY_BUBBLE_VIEW_H_

#include "base/memory/scoped_ptr.h"
#include "ui/views/bubble/bubble_delegate.h"
#include "ui/views/views_export.h"

// Specialized bubble view for bubbles associated with a tray icon (e.g. the
// Ash status area). Mostly this handles custom anchor location and arrow and
// border rendering. This also has its own delegate for handling mouse events
// and other implementation specific details.

namespace ui {
class LocatedEvent;
}

namespace views {
class View;
class Widget;
}

namespace views {

namespace internal {
class TrayBubbleBorder;
class TrayBubbleContentMask;
}

class VIEWS_EXPORT TrayBubbleView : public views::BubbleDelegateView {
 public:
  enum AnchorType {
    ANCHOR_TYPE_TRAY,
    ANCHOR_TYPE_BUBBLE
  };

  enum AnchorAlignment {
    ANCHOR_ALIGNMENT_BOTTOM,
    ANCHOR_ALIGNMENT_LEFT,
    ANCHOR_ALIGNMENT_RIGHT
  };

  class VIEWS_EXPORT Delegate {
   public:
    typedef TrayBubbleView::AnchorType AnchorType;
    typedef TrayBubbleView::AnchorAlignment AnchorAlignment;

    Delegate() {}
    virtual ~Delegate() {}

    // Called when the view is destroyed. Any pointers to the view should be
    // cleared when this gets called.
    virtual void BubbleViewDestroyed() = 0;

    // Called when the mouse enters/exits the view.
    virtual void OnMouseEnteredView() = 0;
    virtual void OnMouseExitedView() = 0;

    // Called from GetAccessibleState(); should return the appropriate
    // accessible name for the bubble.
    virtual string16 GetAccessibleNameForBubble() = 0;

    // Passes responsibility for BubbleDelegateView::GetAnchorRect to the
    // delegate.
    virtual gfx::Rect GetAnchorRect(views::Widget* anchor_widget,
                                    AnchorType anchor_type,
                                    AnchorAlignment anchor_alignment) = 0;

    // Called when a bubble wants to hide/destroy itself (e.g. last visible
    // child view was closed).
    virtual void HideBubble(const TrayBubbleView* bubble_view) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  struct VIEWS_EXPORT InitParams {
    static const int kArrowDefaultOffset;

    InitParams(AnchorType anchor_type,
               AnchorAlignment anchor_alignment,
               int min_width,
               int max_width);
    AnchorType anchor_type;
    AnchorAlignment anchor_alignment;
    int min_width;
    int max_width;
    int max_height;
    bool can_activate;
    bool close_on_deactivate;
    SkColor arrow_color;
    views::BubbleBorder::ArrowLocation arrow_location;
    int arrow_offset;
    views::BubbleBorder::Shadow shadow;
  };

  // Constructs and returns a TrayBubbleView. init_params may be modified.
  static TrayBubbleView* Create(gfx::NativeView parent_window,
                                views::View* anchor,
                                Delegate* delegate,
                                InitParams* init_params);

  virtual ~TrayBubbleView();

  // Sets up animations, and show the bubble. Must occur after CreateBubble()
  // is called.
  void InitializeAndShowBubble();

  // Called whenever the bubble size or location may have changed.
  void UpdateBubble();

  // Sets the maximum bubble height and resizes the bubble.
  void SetMaxHeight(int height);

  // Sets the bubble width.
  void SetWidth(int width);

  // Sets whether or not to paint the bubble border arrow.
  void SetPaintArrow(bool paint_arrow);

  // Returns the border insets. Called by TrayEventFilter.
  gfx::Insets GetBorderInsets() const;

  // Called when the delegate is destroyed.
  void reset_delegate() { delegate_ = NULL; }

  Delegate* delegate() { return delegate_; }

  void set_gesture_dragging(bool dragging) { is_gesture_dragging_ = dragging; }
  bool is_gesture_dragging() const { return is_gesture_dragging_; }

  // Overridden from views::WidgetDelegate.
  virtual bool CanActivate() const OVERRIDE;
  virtual views::NonClientFrameView* CreateNonClientFrameView(
      views::Widget* widget) OVERRIDE;
  virtual bool WidgetHasHitTestMask() const OVERRIDE;
  virtual void GetWidgetHitTestMask(gfx::Path* mask) const OVERRIDE;

  // Overridden from views::BubbleDelegateView.
  virtual gfx::Rect GetAnchorRect() OVERRIDE;

  // Overridden from views::View.
  virtual gfx::Size GetPreferredSize() OVERRIDE;
  virtual gfx::Size GetMaximumSize() OVERRIDE;
  virtual void OnMouseEntered(const ui::MouseEvent& event) OVERRIDE;
  virtual void OnMouseExited(const ui::MouseEvent& event) OVERRIDE;
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;

 protected:
  TrayBubbleView(gfx::NativeView parent_window,
                 views::View* anchor,
                 Delegate* delegate,
                 const InitParams& init_params);

  // Overridden from views::BubbleDelegateView.
  virtual void Init() OVERRIDE;

  // Overridden from views::View.
  virtual void ChildPreferredSizeChanged(View* child) OVERRIDE;
  virtual void ViewHierarchyChanged(bool is_add,
                                    views::View* parent,
                                    views::View* child) OVERRIDE;

 private:
  InitParams params_;
  Delegate* delegate_;
  int preferred_width_;
  internal::TrayBubbleBorder* bubble_border_;
  scoped_ptr<internal::TrayBubbleContentMask> bubble_content_mask_;
  bool is_gesture_dragging_;

  DISALLOW_COPY_AND_ASSIGN(TrayBubbleView);
};

}  // namespace views

#endif  // UI_VIEWS_BUBBLE_TRAY_BUBBLE_VIEW_H_
