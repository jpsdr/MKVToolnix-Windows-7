#include "common/common_pch.h"

#import <AppKit/AppKit.h>

#include <QWidget>

#include "mkvtoolnix-gui/jobs/model.h"
#include "mkvtoolnix-gui/jobs/tool.h"
#include "mkvtoolnix-gui/main_window/main_window.h"
#include "mkvtoolnix-gui/main_window/taskbar_progress.h"

// Custom progress bar view - added as subview of NSImageView at Y=0 (bottom)
@interface DockProgressBar : NSView
@property (nonatomic) double progress;  // 0.0 to 100.0
@end

@implementation DockProgressBar

- (void)drawRect:(NSRect)dirtyRect {
  [super drawRect:dirtyRect];

  [NSGraphicsContext saveGraphicsState];

  // Add subtle drop shadow
  NSShadow *shadow = [[NSShadow alloc] init];
  [shadow setShadowColor:[NSColor colorWithWhite:0.0 alpha:0.5]];
  [shadow setShadowOffset:NSMakeSize(0, -1)];
  [shadow setShadowBlurRadius:3.0];
  [shadow set];

  // Draw semi-transparent dark background fill
  NSRect rect = NSInsetRect(self.bounds, 1.0, 1.0);
  CGFloat radius = rect.size.height / 2;
  NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:rect
                                                       xRadius:radius
                                                       yRadius:radius];
  [[NSColor colorWithWhite:0.0 alpha:0.6] set];
  [path fill];

  [NSGraphicsContext restoreGraphicsState];

  // Draw darker gray border
  [path setLineWidth:1.5];
  [[NSColor colorWithWhite:0.3 alpha:1.0] set];
  [path stroke];

  // Inner area for progress fill
  rect = NSInsetRect(rect, 2.0, 2.0);
  radius = rect.size.height / 2;
  path = [NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius];
  [path addClip];

  // Fill progress portion (system accent color)
  rect.size.width = floor(rect.size.width * self.progress / 100.0);
  [[NSColor controlAccentColor] set];
  NSRectFill(rect);
}

@end

// ------------------------------------------------------------

namespace mtx::gui {

class TaskbarProgressPrivate {
  friend class TaskbarProgress;

  NSDockTile *m_dockTile{nil};
  NSImageView *m_imageView{nil};
  DockProgressBar *m_progressBar{nil};
  bool m_running{};
};

TaskbarProgress::TaskbarProgress(QWidget *parent)
  : QObject{parent}
  , p_ptr{new TaskbarProgressPrivate}
{
  auto model = MainWindow::get()->jobTool()->model();

  connect(MainWindow::get(), &MainWindow::windowShown,         this, &TaskbarProgress::setup);
  connect(model,             &Jobs::Model::progressChanged,    this, &TaskbarProgress::updateTaskbarProgress);
  connect(model,             &Jobs::Model::queueStatusChanged, this, &TaskbarProgress::updateTaskbarStatus);
}

TaskbarProgress::~TaskbarProgress() {
  auto &p = *p_func();

  if (p.m_progressBar) {
    [p.m_progressBar release];
    p.m_progressBar = nil;
  }
  if (p.m_imageView) {
    [p.m_imageView release];
    p.m_imageView = nil;
  }
}

void
TaskbarProgress::setup() {
  auto &p = *p_func();

  // Get the dock tile from the application
  p.m_dockTile = [[NSApplication sharedApplication] dockTile];
}

void
TaskbarProgress::updateTaskbarProgress([[maybe_unused]] int progress,
                                       int totalProgress) {
  auto &p = *p_func();

  if (!p.m_running || !p.m_progressBar || !p.m_dockTile)
    return;

  // Update the progress value
  p.m_progressBar.progress = totalProgress;

  // Mark the view as needing redraw
  [p.m_progressBar setNeedsDisplay:YES];

  // Update the dock tile
  [p.m_dockTile display];
}

void
TaskbarProgress::updateTaskbarStatus(Jobs::QueueStatus status) {
  auto &p         = *p_func();
  auto newRunning = Jobs::QueueStatus::Stopped != status;

  if (!p.m_dockTile || (p.m_running == newRunning))
    return;

  p.m_running = newRunning;

  if (newRunning) {
    // Create NSImageView with app icon as dock tile content view
    p.m_imageView = [[NSImageView alloc] init];
    [p.m_imageView setImage:[NSApp applicationIconImage]];
    [p.m_dockTile setContentView:p.m_imageView];

    // Create progress bar as subview at Y=0 (bottom of dock tile)
    CGFloat barHeight = 15.0;
    p.m_progressBar = [[DockProgressBar alloc] initWithFrame:
      NSMakeRect(0, 0, p.m_dockTile.size.width, barHeight)];
    p.m_progressBar.progress = 0;
    [p.m_imageView addSubview:p.m_progressBar];
  } else {
    // Remove progress view, restore normal dock icon
    [p.m_dockTile setContentView:nil];

    // Release views
    if (p.m_progressBar) {
      [p.m_progressBar release];
      p.m_progressBar = nil;
    }
    if (p.m_imageView) {
      [p.m_imageView release];
      p.m_imageView = nil;
    }
  }

  // Update the dock tile
  [p.m_dockTile display];
}

}
