# Frame-Rate Aware Output Scheduling

This document describes the new frame-rate aware output scheduling system that allows multiple outputs with different framerates to coexist efficiently without starvation.

## Overview

The previous system evaluated all outputs as fast as possible in each frame, which led to:
- Inefficient resource usage
- Output starvation (60 FPS outputs would starve 30 FPS outputs)
- Synchronization issues between outputs with different framerates

The new system uses PTS (Presentation Time Stamps) with a 90kHz timebase to schedule outputs individually based on their configured framerates.

## Key Components

### 1. PTS Clock (`src/vortex/sync/pts_clock.h`)

Provides master timing using a 90kHz timebase (standard for video timing):
- `CurrentPTS()` - Get the current presentation timestamp
- `NextFramePTS()` - Calculate when the next frame should be presented
- `TimeUntilPTS()` - Calculate how long to wait for a target PTS
- `RoundToFrameBoundary()` - Align PTS to frame boundaries

### 2. Output Scheduler (`src/vortex/graph/output_scheduler.h`)

Manages frame-rate aware scheduling of multiple outputs:
- Tracks each output's next presentation time
- Determines which outputs are ready for evaluation
- Provides waiting mechanisms for proper timing
- Maintains evaluation order based on PTS

### 3. Modified Graph Model (`src/vortex/model.h`)

The `TraverseNodes()` method now:
1. Updates the scheduler with current outputs
2. Gets outputs ready for evaluation (based on PTS)
3. Evaluates only ready outputs
4. Marks outputs as presented after evaluation

## Usage Example

```cpp
// Create outputs with different framerates
auto window_60fps = model.CreateNode(gfx, "WindowOutput", {}, {
    {"framerate", "60/1"}
});

auto ndi_30fps = model.CreateNode(gfx, "NDIOutput", {}, {
    {"framerate", "30/1"}  
});

// The system will automatically:
// - Evaluate window output every ~16.67ms (60 FPS)
// - Evaluate NDI output every ~33.33ms (30 FPS)
// - Prevent the 60 FPS output from starving the 30 FPS output

// Main rendering loop
while (running) {
    model.TraverseNodes(gfx, probe);  // Now respects individual framerates
}
```

## Key Benefits

### 1. Efficient Resource Usage
- Outputs are only evaluated when needed
- No wasted computation on outputs that aren't ready for a new frame

### 2. Proper Framerate Handling
- Each output maintains its configured framerate precisely
- No output starvation regardless of relative framerates

### 3. Blocking vs Non-blocking Outputs
- **Window outputs**: Non-blocking, return immediately after presentation
- **NDI outputs**: May block during presentation to maintain timing
- Both are handled correctly by the scheduler

### 4. Master Clock Synchronization
- All outputs are synchronized to a master PTS clock
- Consistent timing relationships between outputs
- Easy synchronization reset with `ResetTiming()`

## Implementation Details

### PTS Timebase
The 90kHz timebase is chosen because:
- Standard in video systems (MPEG, broadcast)
- High enough resolution for precise timing
- Integer arithmetic friendly for most common framerates

### Output Evaluation Flow
1. **Scheduling**: Determine which outputs need frames based on PTS
2. **Evaluation**: Process only ready outputs
3. **Presentation**: Handle blocking/non-blocking presentation
4. **Next Frame**: Schedule the next evaluation time

### Thread Safety
The current implementation is single-threaded but designed to be easily extended for multi-threading if needed in the future.

## Migration from Old System

### Before
```cpp
void TraverseNodes() {
    // Evaluate all outputs every frame
    for (auto* output : outputs) {
        output->Evaluate();  // Runs at maximum possible rate
    }
}
```

### After
```cpp
void TraverseNodes() {
    // Only evaluate outputs that are ready based on their framerate
    auto ready_outputs = scheduler.GetAllReadyOutputs();
    for (auto* output : ready_outputs) {
        scheduler.MarkOutputEvaluated(output);
        output->Evaluate();  // Runs at configured framerate
        scheduler.MarkOutputPresented(output);
    }
}
```

## Testing

The system includes comprehensive tests covering:
- Basic scheduling functionality
- Multiple outputs with different framerates
- Edge cases and timing boundaries
- Compatibility with existing output types

## Future Enhancements

Potential improvements include:
- Multi-threaded evaluation for independent outputs
- Dynamic framerate adjustment
- More sophisticated timing prediction
- Integration with GPU performance metrics