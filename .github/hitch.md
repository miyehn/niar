# Intermittent FIFO Present Hitch

## Status

Deferred investigation. The hitch is documented here so lighting work can continue without keeping temporary timing UI in the app.

## Symptom

- WASD camera movement can feel laggy or jittery for a while, then recover.
- Dragging the window across monitors seems able to trigger it.
- The visible frame rate can remain around 60 FPS while max frame times jump into roughly the 40-60 ms range.

## Evidence Captured

Temporary instrumentation split the frame wait into fence, acquire, swapchain-image, and present waits.

One captured FIFO-relaxed hitch showed:

```text
Frame max: 50.41 ms
Draw max: 50.35 ms
Wait max: 49.25 ms
Fence max: 31.96 ms
Acquire max: 0.04 ms
Image max: 49.15 ms
Present max: 0.14 ms
```

Earlier FIFO captures had the same shape: the large draw/frame max was mostly Vulkan wait time, and the largest bucket was the wait for `imagesInFlight[currentImageIndex]` after `vkAcquireNextImageKHR`.

Interpretation: the app acquired a swapchain image, then had to wait for the fence associated with the previous submission that used that image. Acquire and present calls themselves were not the slow calls in the captured hitch.

## Present Mode Experiments

The `Debug.PresentMode` config switch remains in `config/global.ini`.

Observed behavior:

- `fifo`: hitch reproduced.
- `fifo_relaxed`: hitch reproduced with the same image-fence wait shape.
- `mailbox`: no obvious hitch observed, but the app ran uncapped around 1300 FPS.
- `immediate`: no obvious hitch observed, but also ran uncapped and is not desirable for normal development.

Working theory: the hitch is tied to FIFO-style present/compositor pacing, possibly made worse by monitor moves or mixed-monitor behavior. Mailbox/immediate avoid the problematic pacing path, but need an app-side frame cap if used for development.

## Possible Follow-Ups

- Add a `Debug.TargetFps` frame cap and test `PresentMode: "mailbox"` with a sane cap.
- Revisit swapchain synchronization and the `imagesInFlight[currentImageIndex]` fence wait.
- Test on same-refresh monitors and mixed-refresh monitors.
- Query and log available present modes on startup.
- Investigate `VK_KHR_present_mode_fifo_latest_ready` / `VK_EXT_present_mode_fifo_latest_ready`.

`VK_PRESENT_MODE_FIFO_LATEST_READY_EXT` is a tear-free FIFO-like mode where the presentation engine can dequeue multiple ready present requests during vblank and display the latest ready image. It is closer to the behavior wanted here than plain FIFO, but support is extension-dependent and it requires enabling the device extension/feature before using the mode.

References:

- https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_present_mode_fifo_latest_ready.html
- https://docs.vulkan.org/features/latest/features/proposals/VK_EXT_present_mode_fifo_latest_ready.html
