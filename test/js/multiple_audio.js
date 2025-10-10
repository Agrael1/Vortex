async function testCreateNode() {

    var outputNDI = await Vortex.CreateNodeAsync("NDIOutput");
    Vortex.SetNodePropertyByName(outputNDI, "name", "Test Source");
    Vortex.SetNodePropertyByName(outputNDI, "window_size", "[1920,1080]");
    Vortex.SetNodePropertyByName(outputNDI, "framerate", "[30,1]");
    Vortex.SetNodeInfo(outputNDI, "Output 1");

    var outputNDI2 = await Vortex.CreateNodeAsync("NDIOutput");
    Vortex.SetNodePropertyByName(outputNDI2, "name", "Test Source2");
    Vortex.SetNodePropertyByName(outputNDI2, "window_size", "[1920,1080]");
    Vortex.SetNodePropertyByName(outputNDI2, "framerate", "[60,1]");
    Vortex.SetNodeInfo(outputNDI2, "Output 2");

    var streamSource = await Vortex.CreateNodeAsync("StreamInput");
    Vortex.SetNodePropertyByName(streamSource, "stream_url", "rtp://192.168.100.5:6970");

    await Vortex.ConnectNodesAsync(streamSource, 0, outputNDI, 0);
    await Vortex.ConnectNodesAsync(streamSource, 0, outputNDI2, 0);
    await Vortex.ConnectNodesAsync(streamSource, 1, outputNDI, 1);
    await Vortex.ConnectNodesAsync(streamSource, 1, outputNDI2, 1);
    Vortex.Play();
}

testCreateNode();