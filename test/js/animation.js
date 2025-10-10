async function f() {
    var outputNDI = await Vortex.CreateNodeAsync("NDIOutput");
    Vortex.SetNodePropertyByName(outputNDI, "name", "Test Source");
    Vortex.SetNodePropertyByName(outputNDI, "window_size", "[1920,1080]");
    Vortex.SetNodePropertyByName(outputNDI, "framerate", "[60,1]");
    Vortex.SetNodeInfo(outputNDI, "Output 1");

    var streamSource = await Vortex.CreateNodeAsync("StreamInput");
    Vortex.SetNodePropertyByName(streamSource, "stream_url", "rtp://192.168.100.5:6970");

    var selector = await Vortex.CreateNodeAsync("Select");
    var imageSource = await Vortex.CreateNodeAsync("ImageInput");
    var watermark = await Vortex.CreateNodeAsync("ImageInput");
    Vortex.SetNodePropertyByName(imageSource, "image_path", "ui/HDR.jpg");
    Vortex.SetNodePropertyByName(watermark, "image_path", "ui/watermark.png");

    var blend = await Vortex.CreateNodeAsync("Blend");

    await Vortex.ConnectNodesAsync(streamSource, 0, selector, 0);
    await Vortex.ConnectNodesAsync(imageSource, 0, blend, 0);
    await Vortex.ConnectNodesAsync(watermark, 0, blend, 1);

    await Vortex.ConnectNodesAsync(blend, 0, selector, 1);

    await Vortex.ConnectNodesAsync(selector, 0, outputNDI, 0);
    await Vortex.ConnectNodesAsync(streamSource, 1, outputNDI, 1);

    var anim = await Vortex.CreateAnimationAsync(selector);
    var obj = { "time_from_start": 450000, "value": 1 };
    var track = await Vortex.AddPropertyTrackAsync(anim, "input_index", "");
    Vortex.AddKeyframe(track, JSON.stringify(obj));

    Vortex.Play();
}

f();